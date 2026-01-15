import { useState, useEffect } from 'react';
import { X, Search, UserPlus, Users, School } from 'lucide-react';
import { invoke } from '@tauri-apps/api/core';
import { sendMessageWS } from '../../utils/websocket';
import { getTIMGroups, addMessageListener } from '../../utils/tim';

interface SearchAddModalProps {
    isOpen: boolean;
    onClose: () => void;
    userInfo?: any;
}

type SearchTab = 'all' | 'class' | 'teacher' | 'group';

interface SearchResult {
    type: 'class' | 'teacher' | 'group';
    data: any;
}

const SearchAddModal = ({ isOpen, onClose, userInfo }: SearchAddModalProps) => {
    const [activeTab, setActiveTab] = useState<SearchTab>('all');
    const [keyword, setKeyword] = useState('');
    const [results, setResults] = useState<SearchResult[]>([]);
    const [isLoading, setIsLoading] = useState(false);
    const [searched, setSearched] = useState(false);
    const [myFriends, setMyFriends] = useState<string[]>([]);
    const [myGroups, setMyGroups] = useState<string[]>([]);
    const [myClasses, setMyClasses] = useState<string[]>([]);
    const [currentUserId, setCurrentUserId] = useState('');

    useEffect(() => {
        const fetchRelations = async () => {
            if (!isOpen) return;

            const userId = localStorage.getItem('unique_id') || localStorage.getItem('userid') || localStorage.getItem('teacher_unique_id') || '';
            setCurrentUserId(userId);
            const idCard = localStorage.getItem('id_number') || localStorage.getItem('id_card') || '';
            const token = localStorage.getItem('token') || '';

            // 1. Fetch Friends (Requires id_card and token)
            if (idCard && token) {
                try {
                    const res = await invoke<string>('get_user_friends', { idCard, token });
                    console.log("[Search] get_user_friends raw:", res);
                    const json = JSON.parse(res);
                    console.log("[Search] get_user_friends Parsed JSON:", JSON.stringify(json, null, 2));
                    // Check structure based on logs or standard format
                    // Usually: { code: 200, data: { friends: [...] } } or { code: 200, data: [...] }
                    // Logic from SearchDialog.h implies { friends: [...] } might be in data? 
                    // Let's safe crawl.
                    if (json.friends && Array.isArray(json.friends)) {
                        const ids = json.friends.map((f: any) => {
                            console.log("[Search] Processing Friend Item:", JSON.stringify(f));
                            // Match ClassManagement.tsx logic: teacher_info or user_details
                            if (f.teacher_info && f.teacher_info.teacher_unique_id) {
                                console.log(`[Search] Found teacher_unique_id in teacher_info: ${f.teacher_info.teacher_unique_id} `);
                                return String(f.teacher_info.teacher_unique_id);
                            }
                            // Some friends might not be teachers, check user_details or other fields if needed, 
                            // but for "Teacher" search check, we primarily need teacher_unique_id.
                            // User request specifically mentioned teacher_unique_id comparison.
                            return f.user_details ? String(f.user_details.id || f.user_details.id_number) : null;
                        }).filter(Boolean);
                        setMyFriends(ids);
                        console.log("[Search] myFriends (Parsed from friends field):", ids);
                    } else if (json.data && Array.isArray(json.data)) {
                        // Fallback for different response structure if any
                        const ids = json.data.map((f: any) => {
                            if (f.teacher_unique_id) return String(f.teacher_unique_id);
                            if (f.teacher_info && f.teacher_info.teacher_unique_id) return String(f.teacher_info.teacher_unique_id);
                            return String(f.unique_id || f.friend_id);
                        }).filter(Boolean);
                        setMyFriends(ids);
                        console.log("[Search] myFriends (Parsed from data field):", ids);
                    }
                } catch (e) {
                    console.error("Failed to fetch friends for check", e);
                }
            } else {
                console.warn("[Search] Skipping friend fetch. Missing credentials. idCard:", idCard, "Token present:", !!token);
            }

            // 2. Fetch Joined Groups (TIM)
            try {
                const groups = await getTIMGroups();
                if (groups) {
                    const groupIds = groups.map((g: any) => g.groupID);
                    setMyGroups(groupIds);
                    console.log("[Search] myGroups (TIM):", groupIds);
                }
            } catch (e) {
                console.error("Failed to fetch groups for check", e);
            }

            // 3. Fetch Joined Classes (Requires teacher_unique_id and token)
            if (userId && token) {
                try {
                    const res = await invoke<string>('get_teacher_classes', { teacherUniqueId: userId, token });
                    console.log("[Search] get_teacher_classes raw:", res);
                    const json = JSON.parse(res);
                    // Assuming structure
                    let list: any[] = [];
                    if (json.data && Array.isArray(json.data)) list = json.data;
                    else if (json.data && json.data.classes && Array.isArray(json.data.classes)) list = json.data.classes;

                    if (list.length > 0) {
                        const codes = list.map((c: any) => String(c.class_code || c.class_id)); // Use class_code for matching usually? Or class_id? Search uses class_code or id.
                        setMyClasses(codes);
                        console.log("[Search] myClasses:", codes);
                    }
                } catch (e) {
                    console.error("Failed to fetch classes for check", e);
                }
            }
        };

        if (isOpen) {
            setKeyword('');
            setResults([]);
            setSearched(false);
            fetchRelations();
        }

        const handleWSMessage = (event: Event) => {
            const customEvent = event as CustomEvent;
            try {
                const msg = customEvent.detail;
                const data = JSON.parse(msg);
                if (data.type === 'add_friend_success' && data.friend_teacher_unique_id) {
                    console.log("[Search] Received friend add success:", data.friend_teacher_unique_id);
                    setMyFriends(prev => {
                        if (prev.includes(String(data.friend_teacher_unique_id))) return prev;
                        return [...prev, String(data.friend_teacher_unique_id)];
                    });
                    // Notify user of success
                    alert(`${data.message || "添加好友成功"} `);
                }
            } catch (e) {
                // Ignore non-JSON messages
            }
        };

        window.addEventListener('ws-message', handleWSMessage);

        // TIM Message Listener for Group Join Success
        const onTIMMessage = (event: any) => {
            const messages = event.data || [];
            messages.forEach((msg: any) => {
                if (msg.type === 'TIMGroupSystemNoticeElem') {
                    const opType = msg.payload.operationType;
                    // 7 = Invited to group (Direct Add), 2 = Application Accepted
                    if (opType === 7 || opType === 2) {
                        const groupProfile = msg.payload.groupProfile;
                        if (groupProfile && groupProfile.groupID) {
                            console.log("[Search] Group Join Success:", groupProfile.groupID);
                            setMyGroups(prev => {
                                if (prev.includes(groupProfile.groupID)) return prev;
                                return [...prev, String(groupProfile.groupID)];
                            });
                            const groupName = groupProfile.name || groupProfile.to || "群组"; // groupProfile usually has 'name' or we use ID
                            alert(`加入群组 "${groupName}" 成功`);
                        }
                    }
                }
            });
        };

        const removeTIMListener = addMessageListener(onTIMMessage);

        return () => {
            window.removeEventListener('ws-message', handleWSMessage);
            removeTIMListener();
        };
    }, [isOpen]);

    const handleSearch = async () => {
        console.log("[Search] handleSearch called. Current myFriends:", myFriends); // Debug: Check if friends list is populated
        if (!keyword.trim()) return;
        setIsLoading(true);
        setSearched(true);
        setResults([]);

        console.log(`[Search] Starting search.Keyword: "${keyword}", Tab: "${activeTab}"`);

        try {
            const newResults: SearchResult[] = [];

            // Parallel requests based on tab
            const tasks = [];

            // 1. Teachers
            if (activeTab === 'all' || activeTab === 'teacher') {
                tasks.push(
                    invoke<string>('search_teachers', { keyword }).then(res => {
                        console.log('[Search] Teachers Raw:', res);
                        try {
                            const json = JSON.parse(res);
                            console.log('[Search] Teachers Parsed:', json);
                            const data = json.data || json;
                            const list = data.teachers || (Array.isArray(data.data) ? data.data : []);

                            if ((data.code === 200 || json.code === 200) && Array.isArray(list)) {
                                list.forEach((item: any) => newResults.push({ type: 'teacher', data: item }));
                            } else {
                                console.warn('[Search] Teachers response format mismatch:', json);
                            }
                        } catch (e) {
                            console.error("[Search] Teachers Parse Error:", e);
                        }
                    }).catch(e => console.error("[Search] Teachers Request Failed:", e))
                );
            }

            // 2. Classes
            if (activeTab === 'all' || activeTab === 'class') {
                tasks.push(
                    invoke<string>('search_classes', { keyword }).then(res => {
                        console.log('[Search] Classes Raw:', res);
                        try {
                            const json = JSON.parse(res);
                            console.log('[Search] Classes Parsed:', json);
                            const data = json.data || json;
                            // Likely 'classes' or just 'data' array
                            const list = data.classes || data.data || (Array.isArray(data) ? data : []);

                            if ((data.code === 200 || json.code === 200) && Array.isArray(list)) {
                                list.forEach((item: any) => newResults.push({ type: 'class', data: item }));
                            } else {
                                console.warn('[Search] Classes response format mismatch:', json);
                            }
                        } catch (e) {
                            console.error("[Search] Classes Parse Error:", e);
                        }
                    }).catch(e => console.error("[Search] Classes Request Failed:", e))
                );
            }

            // 3. Groups (Class Groups)
            if (activeTab === 'all' || activeTab === 'group') {
                tasks.push(
                    invoke<string>('search_class_groups', { keyword }).then(res => {
                        console.log('[Search] Groups Raw:', res);
                        try {
                            const json = JSON.parse(res);
                            console.log('[Search] Groups Parsed:', json);
                            const data = json.data || json;
                            // Likely 'groups'
                            const list = data.groups || data.group_list || (Array.isArray(data.data) ? data.data : []);

                            if ((data.code === 200 || json.code === 200) && Array.isArray(list)) {
                                list.forEach((item: any) => newResults.push({ type: 'group', data: item }));
                            } else {
                                console.warn('[Search] Groups response format mismatch:', json);
                            }
                        } catch (e) {
                            console.error("[Search] Groups Parse Error:", e);
                        }
                    }).catch(e => console.error("[Search] Groups Request Failed:", e))
                );
            }

            await Promise.all(tasks);
            setResults(newResults);

        } catch (error) {
            console.error("Search failed:", error);
        } finally {
            setIsLoading(false);
        }
    };

    const handleAddFriend = async (teacher: any) => {
        // Use WebSocket to send friend request
        // Format: to:target_id:{ "type": "add_friend_request", "from_id": myId, "from_name": myName, "text": "Reason" }
        // Simplified: Just msg body?
        // Legacy: "to:target_id:{\"type\":10, ...}" ? 
        // Logic from `SearchDialog.cpp`: 
        // QString msg = QString("type=addFriend&from=%1&text=%2").arg(myId).arg(reason);
        // Wrapper `TaQTWebSocket` sends it.
        // Wait, legacy logic `onAddFriendClicked` uses `client -> sendTextMessage("to:" + targetId + ":" + jsonStr)`.

        // Let's assume standard format needed.
        const userId = localStorage.getItem('unique_id') || localStorage.getItem('userid') || localStorage.getItem('teacher_unique_id');
        const userName = localStorage.getItem('username') || localStorage.getItem('name') || localStorage.getItem('strName') || "Unknown";

        if (!userId) {
            console.error("User ID not found in localStorage. Available keys:", Object.keys(localStorage));
            alert("无法获取当前用户信息，请尝试重新登录");
            return;
        }

        const reason = prompt("请输入验证消息:");
        if (reason === null) return; // Cancelled

        const msgObj = {
            type: "1",
            teacher_unique_id: teacher.teacher_unique_id,
            text: reason
        };

        const wsMsg = `to:${teacher.teacher_unique_id}:${JSON.stringify(msgObj)} `;

        try {
            // Using existing WS
            // We need to import `sendWSMessage` from `websocket.ts`? 
            // Or use Tauri event?
            // The plan said "Reuse src/utils/websocket.ts".
            // I'll import it.
            sendMessageWS(wsMsg);
            // alert("好友请求已发送"); // User requested to remove this "sent" confirmation
        } catch (e) {
            console.error("Failed to send friend request", e);
            alert("发送失败");
        }
    };

    const handleJoinGroup = async (group: any) => {
        const userId = userInfo?.teacher_unique_id || localStorage.getItem('unique_id') || localStorage.getItem('userid') || localStorage.getItem('teacher_unique_id');
        const userName = userInfo?.name || localStorage.getItem('username') || localStorage.getItem('name') || "Unknown";

        if (!userId) {
            alert("无法获取当前用户信息，请尝试重新登录");
            return;
        }

        // Use default reason instead of prompt to avoid UI blocking issues
        const reason = "申请加入";
        console.log("[Search] Joining group with reason:", reason);

        try {
            const res = await invoke<string>('join_class_group_request', {
                groupId: group.group_id,
                userId: userId,
                userName: userName,
                reason: reason
            });
            console.log("[Search] Join request response:", res);

            try {
                const data = JSON.parse(res);
                if (data.code === 200) {
                    // Since user complained about "No reaction", we show the server message
                    alert(data.message || "申请已提交");
                } else if (data.code === 400 && (String(data.message).includes("已经") || String(data.message).includes("already"))) {
                    // User matches logic for "Already in group"
                    // Update UI to reflect this
                    setMyGroups(prev => {
                        const strId = String(group.group_id);
                        if (!prev.includes(strId)) return [...prev, strId];
                        return prev;
                    });
                    alert("您已加入该群组");
                } else {
                    alert(data.message || "操作失败");
                }
            } catch (jsonErr) {
                console.error("Failed to parse join response", jsonErr);
                // If not JSON, maybe just success text? Or ignore?
                // Just log it.
            }

        } catch (e) {
            console.error("Join group failed", e);
            alert(`申请失败: ${e} `);
        }
    };

    const handleJoinClass = async (classItem: any) => {
        const userId = localStorage.getItem('unique_id') || localStorage.getItem('userid') || localStorage.getItem('teacher_unique_id');
        if (!userId) {
            alert("无法获取当前用户信息");
            return;
        }

        const classCode = classItem.class_code || classItem.class_id || String(classItem.id);

        try {
            await invoke('join_class', {
                teacherUniqueId: userId,
                classCode: classCode
            });
            alert("加入班级成功");
            // Update local state to grey out button immediately
            setMyClasses(prev => [...prev, String(classCode)]);
        } catch (e: any) {
            console.error("Join class failed", e);
            alert(`加入失败: ${typeof e === 'string' ? e : (e.message || JSON.stringify(e))} `);
        }
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[60] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-200 p-4">
            <div className="bg-white rounded-2xl shadow-2xl w-[800px] max-h-[90vh] flex flex-col overflow-hidden">

                {/* Header */}
                <div className="flex items-center justify-between px-6 py-4 border-b border-gray-100 bg-gradient-to-r from-blue-50 to-white">
                    <div className="flex items-center gap-3">
                        <div className="w-10 h-10 rounded-xl bg-gradient-to-br from-blue-500 to-blue-600 flex items-center justify-center shadow-lg">
                            <Search size={20} className="text-white" />
                        </div>
                        <div>
                            <h2 className="text-xl font-bold text-gray-800">查找添加</h2>
                            <p className="text-xs text-gray-500">搜索班级、老师或群组</p>
                        </div>
                    </div>
                    <button
                        onClick={onClose}
                        className="w-8 h-8 flex items-center justify-center text-gray-400 hover:text-gray-600 hover:bg-gray-100 rounded-full transition-colors"
                    >
                        <X size={20} />
                    </button>
                </div>

                {/* Tabs */}
                <div className="flex bg-gray-50 px-6 py-3 gap-2 border-b border-gray-100">
                    {[
                        { id: 'all', label: '全部', icon: '🔍' },
                        { id: 'class', label: '找班级', icon: '🏫' },
                        { id: 'teacher', label: '找老师', icon: '👨‍🏫' },
                        { id: 'group', label: '找群组/讨论组', icon: '👥' }
                    ].map(tab => (
                        <button
                            key={tab.id}
                            onClick={() => setActiveTab(tab.id as SearchTab)}
                            className={`px - 4 py - 2 rounded - lg text - sm font - medium transition - all duration - 200 flex items - center gap - 2 ${activeTab === tab.id
                                ? 'bg-blue-500 text-white shadow-md shadow-blue-200'
                                : 'text-gray-600 hover:bg-white hover:shadow-sm'
                                } `}
                        >
                            <span>{tab.icon}</span>
                            {tab.label}
                        </button>
                    ))}
                </div>

                {/* Search Bar */}
                <div className="px-6 py-4 bg-white border-b border-gray-100">
                    <div className="flex gap-3">
                        <div className="flex-1 relative">
                            <input
                                type="text"
                                value={keyword}
                                onChange={(e) => setKeyword(e.target.value)}
                                onKeyDown={(e) => e.key === 'Enter' && handleSearch()}
                                placeholder={activeTab === 'teacher' ? "输入老师姓名/ID" : activeTab === 'class' ? "输入班级编号/名称" : "请输入关键字搜索"}
                                className="w-full px-4 py-3 pl-11 border border-gray-200 rounded-xl outline-none focus:ring-2 focus:ring-blue-500/20 focus:border-blue-500 transition-all text-gray-700 placeholder:text-gray-400"
                            />
                            <Search size={18} className="absolute left-4 top-1/2 -translate-y-1/2 text-gray-400" />
                        </div>
                        <button
                            onClick={handleSearch}
                            disabled={isLoading}
                            className="px-6 py-3 bg-gradient-to-r from-blue-500 to-blue-600 text-white rounded-xl hover:from-blue-600 hover:to-blue-700 font-medium flex items-center gap-2 shadow-lg shadow-blue-200 transition-all disabled:opacity-50"
                        >
                            <Search size={18} />
                            查找
                        </button>
                    </div>
                </div>

                {/* Results */}
                <div className="flex-1 overflow-y-auto p-6 bg-gray-50 min-h-[300px]">
                    {isLoading ? (
                        <div className="flex flex-col items-center justify-center h-full text-gray-400 gap-3">
                            <div className="w-8 h-8 border-3 border-blue-500/20 border-t-blue-500 rounded-full animate-spin" />
                            <span className="text-sm">正在搜索...</span>
                        </div>
                    ) : searched && results.length === 0 ? (
                        <div className="flex flex-col items-center justify-center h-full text-gray-400 gap-3">
                            <div className="w-16 h-16 rounded-full bg-gray-100 flex items-center justify-center">
                                <Search size={24} className="text-gray-300" />
                            </div>
                            <span className="text-sm">未找到相关结果</span>
                        </div>
                    ) : !searched ? (
                        <div className="flex flex-col items-center justify-center h-full text-gray-400 gap-3">
                            <div className="w-20 h-20 rounded-full bg-gradient-to-br from-blue-50 to-blue-100 flex items-center justify-center">
                                <Search size={32} className="text-blue-300" />
                            </div>
                            <span className="text-sm">输入关键字开始搜索</span>
                        </div>
                    ) : (
                        <div className="flex flex-col gap-3">
                            {results.map((item, idx) => (
                                <div key={idx} className="bg-white p-4 rounded-xl shadow-sm flex items-center justify-between border border-gray-100 hover:shadow-md hover:border-blue-100 transition-all">
                                    <div className="flex items-center gap-4">
                                        {/* Icon */}
                                        <div className={`w - 12 h - 12 rounded - xl flex items - center justify - center ${item.type === 'teacher' ? 'bg-green-50 text-green-500' :
                                            item.type === 'group' ? 'bg-purple-50 text-purple-500' :
                                                'bg-blue-50 text-blue-500'
                                            } `}>
                                            {item.type === 'teacher' && <UserPlus size={24} />}
                                            {item.type === 'group' && <Users size={24} />}
                                            {item.type === 'class' && <School size={24} />}
                                        </div>

                                        {/* Info */}
                                        <div>
                                            <div className="font-bold text-lg text-gray-800">
                                                {item.type === 'teacher' ? item.data.name :
                                                    item.type === 'group' ? item.data.group_name :
                                                        // For class: combine grade + class_name for full display
                                                        (item.data.grade ? item.data.grade + item.data.class_name : item.data.class_name)}
                                            </div>
                                            <div className="text-xs text-gray-500">
                                                {item.type === 'teacher' ? `ID: ${item.data.teacher_unique_id} | 学校: ${item.data.school_id} ` :
                                                    item.type === 'group' ? `ID: ${item.data.group_id} | 创建者: ${item.data.creator_id} ` :
                                                        // For class: use class_code or class_id, also show school stage if available
                                                        `ID: ${item.data.class_code || item.data.class_id || ''} ${item.data.school_stage ? `| ${item.data.school_stage}` : ''} `}
                                            </div>
                                        </div>
                                    </div>

                                    {/* Action */}
                                    <div>
                                        {item.type === 'teacher' && (
                                            (() => {
                                                const teacherId = String(item.data.teacher_unique_id);
                                                if (teacherId === currentUserId) {
                                                    return (
                                                        <span className="px-3 py-1 bg-gray-300 text-gray-500 rounded text-sm cursor-default">
                                                            你自己
                                                        </span>
                                                    );
                                                }
                                                const isFriend = myFriends.includes(teacherId);
                                                return isFriend ? (
                                                    <span className="px-4 py-2 bg-gray-100 text-gray-400 rounded-lg text-sm cursor-not-allowed">
                                                        已添加
                                                    </span>
                                                ) : (
                                                    <button
                                                        onClick={() => handleAddFriend(item.data)}
                                                        className="px-4 py-2 bg-gradient-to-r from-green-500 to-green-600 text-white rounded-lg hover:from-green-600 hover:to-green-700 text-sm shadow-md shadow-green-200 transition-all"
                                                    >
                                                        加好友
                                                    </button>
                                                );
                                            })()
                                        )}
                                        {item.type === 'group' && (
                                            (() => {
                                                const isJoined = myGroups.includes(String(item.data.group_id));
                                                return isJoined ? (
                                                    <span className="px-4 py-2 bg-gray-100 text-gray-400 rounded-lg text-sm cursor-not-allowed">
                                                        已加入
                                                    </span>
                                                ) : (
                                                    <button
                                                        onClick={() => handleJoinGroup(item.data)}
                                                        className="px-4 py-2 bg-gradient-to-r from-blue-500 to-blue-600 text-white rounded-lg hover:from-blue-600 hover:to-blue-700 text-sm shadow-md shadow-blue-200 transition-all"
                                                    >
                                                        加入群
                                                    </button>
                                                );
                                            })()
                                        )}
                                        {item.type === 'class' && (
                                            (() => {
                                                const classCode = String(item.data.class_code || item.data.class_id);
                                                const isJoined = myClasses.includes(classCode);
                                                return isJoined ? (
                                                    <span className="px-4 py-2 bg-gray-100 text-gray-400 rounded-lg text-sm cursor-not-allowed">
                                                        已加入
                                                    </span>
                                                ) : (
                                                    <button
                                                        onClick={() => handleJoinClass(item.data)}
                                                        className="px-4 py-2 bg-gradient-to-r from-blue-500 to-blue-600 text-white rounded-lg hover:from-blue-600 hover:to-blue-700 text-sm shadow-md shadow-blue-200 transition-all"
                                                    >
                                                        加入
                                                    </button>
                                                );
                                            })()
                                        )}
                                    </div>
                                </div>
                            ))}
                        </div>
                    )}
                </div>

            </div>
        </div>
    );
};

export default SearchAddModal;
