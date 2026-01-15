import { useEffect, useState } from 'react';
import { useParams } from 'react-router-dom';
import { getCurrentWindow } from '@tauri-apps/api/window';
import { Minus, X, Square, Copy, MessageCircle, MoreHorizontal, Image, Paperclip, Smile, Clock, LogOut, Trash2, UserCheck } from 'lucide-react';
import { invoke } from '@tauri-apps/api/core';
import { sendMessage, sendImageMessage, sendFileMessage, getMessageList, loginTIM, isSDKReady, getGroupMemberList, dismissGroup, quitGroup, changeGroupOwner } from '../utils/tim';
import CustomListModal from './modals/CustomListModal';
import { useRef } from 'react';

const ClassChatWindow = () => {
    const { groupclassId } = useParams();
    const [isMaximized, setIsMaximized] = useState(false);
    const [messages, setMessages] = useState<any[]>([]);
    const [inputText, setInputText] = useState('');
    const [isReady, setIsReady] = useState(false);

    const [members, setMembers] = useState<any[]>([]);
    const [showCustomList, setShowCustomList] = useState(false);
    const [showGroupMenu, setShowGroupMenu] = useState(false);
    const [isOwner, setIsOwner] = useState(false);
    const [showTransferModal, setShowTransferModal] = useState(false);

    const imageInputRef = useRef<HTMLInputElement>(null);
    const fileInputRef = useRef<HTMLInputElement>(null);
    const groupMenuRef = useRef<HTMLDivElement>(null);

    // ... (useEffect for window and initChat remains same, skipping lines for brevity)

    useEffect(() => {
        const checkMaximized = async () => {
            const win = getCurrentWindow();
            setIsMaximized(await win.isMaximized());
        };
        checkMaximized();

        const unlisten = getCurrentWindow().onResized(async () => {
            const win = getCurrentWindow();
            setIsMaximized(await win.isMaximized());
        });

        // Initialize TIM if needed
        const initChat = async () => {
            if (!isSDKReady) {
                const teacherId = localStorage.getItem('teacher_unique_id');
                // Ensure Login stores userSig properly, fallback to 'token' might fail if it's not the signature
                const userSig = localStorage.getItem('userSig') || localStorage.getItem('token');

                if (teacherId && userSig) {
                    console.log('ClassChatWindow: Attempting TIM login with', teacherId);
                    await loginTIM(teacherId, userSig);
                } else {
                    console.error('ClassChatWindow: Missing credentials for TIM login');
                }
            }

            // Check readiness in a loop/timeout
            const interval = setInterval(() => {
                if (isSDKReady) {
                    console.log('ClassChatWindow: TIM SDK Ready');
                    setIsReady(true);
                    loadMessages();
                    loadMembers(); // Fetch members
                    clearInterval(interval);
                }
            }, 500);
            setTimeout(() => clearInterval(interval), 10000);
        };
        initChat();

        return () => {
            unlisten.then(f => f());
        }
    }, [groupclassId]);

    const loadMessages = async () => {
        if (!groupclassId) return;
        const list = await getMessageList(groupclassId);
        setMessages(list);
    };

    const loadMembers = async () => {
        if (!groupclassId) return;

        const currentUserId = localStorage.getItem('teacher_unique_id') || '';

        // 1. Fetch TIM Members (includes role info)
        let timList: any[] = [];
        if (isSDKReady) {
            timList = await getGroupMemberList(groupclassId);
        }

        // Check ownership from TIM list
        const selfInTim = timList.find((m: any) => m.userID === currentUserId);
        if (selfInTim && selfInTim.role === 'Owner') {
            setIsOwner(true);
        } else {
            setIsOwner(false);
        }

        // 2. Fetch Backend Members (Primary Source)
        let backendMembers: any[] = [];
        try {
            const userInfoStr = localStorage.getItem('user_info');
            let token = "";
            if (userInfoStr) {
                const u = JSON.parse(userInfoStr);
                token = u.token || "";
            }
            if (token) {
                const resStr = await invoke<string>('get_group_members', {
                    groupId: groupclassId,
                    token
                });
                const res = JSON.parse(resStr);
                if (res.code === 200 || (res.data && res.data.code === 200)) {
                    backendMembers = res.data?.members || res.data || [];
                }
            }
        } catch (e) {
            console.error("Failed to fetch backend members in Chat", e);
        }

        // 3. Merge - Prioritize Backend Members to match ClassInfoModal
        let mergedList: any[] = [];

        if (backendMembers.length > 0) {
            mergedList = backendMembers.map((bm: any) => {
                const userIdStr = String(bm.user_id || bm.id || bm.Member_Account);
                const tm = timList.find((t: any) => t.userID == userIdStr);
                return {
                    userID: userIdStr,
                    nick: bm.user_name || bm.name || bm.student_name || tm?.nick || "未知用户",
                    avatar: tm?.avatar || bm.face_url || bm.avatar,
                    role: tm?.role || bm.role || 'Member',
                };
            });
        } else {
            mergedList = timList.map((tm: any) => ({
                userID: tm.userID,
                nick: tm.nick || tm.userID,
                avatar: tm.avatar,
                role: tm.role,
            }));
        }

        setMembers(mergedList);
    };

    // Handler: Exit Group
    const handleExitGroup = async () => {
        if (!groupclassId) return;

        const currentUserId = localStorage.getItem('teacher_unique_id') || '';

        if (isOwner) {
            // Owner needs to transfer ownership first
            // Find other members (excluding self)
            const otherMembers = members.filter(m => m.userID !== currentUserId);
            if (otherMembers.length === 0) {
                alert('您是群内唯一成员，无法转让群主。请使用"解散群聊"。');
                return;
            }
            setShowTransferModal(true);
        } else {
            // Regular member can just quit
            if (!confirm('确定要退出群聊吗？')) return;
            try {
                await quitGroup(groupclassId);
                alert('已成功退出群聊');
                getCurrentWindow().close();
            } catch (e: any) {
                alert('退出群聊失败: ' + (e.message || e));
            }
        }
        setShowGroupMenu(false);
    };

    // Handler: Transfer Ownership and then Quit (for owner exit)
    const handleTransferAndQuit = async (newOwnerId: string) => {
        if (!groupclassId) return;
        try {
            await changeGroupOwner(groupclassId, newOwnerId);
            await quitGroup(groupclassId);
            alert('已成功转让群主并退出群聊');
            getCurrentWindow().close();
        } catch (e: any) {
            alert('操作失败: ' + (e.message || e));
        }
        setShowTransferModal(false);
    };

    // Handler: Disband Group (Owner only)
    const handleDisbandGroup = async () => {
        if (!groupclassId) return;
        if (!confirm('确定要解散群聊吗？此操作不可恢复！')) return;

        try {
            await dismissGroup(groupclassId);
            // Success - silently close window without alert
            getCurrentWindow().close();
        } catch (e: any) {
            alert('解散群聊失败: ' + (e.message || e));
        }
        setShowGroupMenu(false);
    };

    // Close menu when clicking outside
    useEffect(() => {
        const handleClickOutside = (e: MouseEvent) => {
            if (groupMenuRef.current && !groupMenuRef.current.contains(e.target as Node)) {
                setShowGroupMenu(false);
            }
        };
        if (showGroupMenu) {
            document.addEventListener('mousedown', handleClickOutside);
        }
        return () => document.removeEventListener('mousedown', handleClickOutside);
    }, [showGroupMenu]);

    const handleSendMessage = async () => {
        if (!inputText.trim() || !groupclassId) return;
        try {
            const msg = await sendMessage(groupclassId, inputText);
            if (msg) {
                setMessages(prev => [...prev, msg]);
                setInputText('');
            }
        } catch (e) {
            console.error('Failed to send message', e);
        }
    };

    const handleImageSelect = async (e: React.ChangeEvent<HTMLInputElement>) => {
        console.log('ClassChatWindow: Image selected');
        if (e.target.files && e.target.files[0] && groupclassId) {
            try {
                console.log('ClassChatWindow: Sending Image...', e.target.files[0].name);
                const msg = await sendImageMessage(groupclassId, e.target.files[0]);
                console.log('ClassChatWindow: Image sent result:', msg);
                if (msg) {
                    setMessages(prev => [...prev, msg]);
                } else {
                    alert("发送图片失败：SDK返回空消息");
                }
            } catch (err: any) {
                console.error("Failed to send image", err);
                alert("发送图片失败: " + (err.message || JSON.stringify(err)));
            }
        }
        if (e.target) e.target.value = ''; // Reset input
    };

    const handleFileSelect = async (e: React.ChangeEvent<HTMLInputElement>) => {
        console.log('ClassChatWindow: File selected');
        if (e.target.files && e.target.files[0] && groupclassId) {
            try {
                console.log('ClassChatWindow: Sending File...', e.target.files[0].name);
                const msg = await sendFileMessage(groupclassId, e.target.files[0]);
                console.log('ClassChatWindow: File sent result:', msg);
                if (msg) {
                    setMessages(prev => [...prev, msg]);
                } else {
                    alert("发送文件失败：SDK返回空消息");
                }
            } catch (err: any) {
                console.error("Failed to send file", err);
                alert("发送文件失败: " + (err.message || JSON.stringify(err)));
            }
        }
        if (e.target) e.target.value = ''; // Reset input
    };

    const handleKeyDown = (e: React.KeyboardEvent) => {
        if (e.key === 'Enter' && !e.shiftKey) {
            e.preventDefault();
            handleSendMessage();
        }
    };

    const renderMessageContent = (msg: any) => {
        // Normalize type check
        const type = msg.type || msg.elements?.[0]?.type;
        const payload = msg.payload || msg.elements?.[0]?.content || msg.elements?.[0]?.payload;

        if (type === 'TIMTextElem' || type === 'TIMText') {
            // Handle both structure nuances if any
            const text = payload?.text || payload;
            return (
                <div className={`p-2.5 px-4 rounded-2xl text-sm leading-relaxed shadow-sm break-words ${msg.flow === 'out' ? 'bg-blue-600 text-white rounded-tr-sm' : 'bg-white text-gray-800 rounded-tl-sm border border-gray-100'}`}>
                    {typeof text === 'string' ? text : '[文本解析错误]'}
                </div>
            );
        } else if (type === 'TIMImageElem' || type === 'TIMImage') {
            const imageInfoArray = payload?.imageInfoArray || payload?.imageArray; // defensive
            const imgUrl = imageInfoArray?.[0]?.url || imageInfoArray?.[1]?.url;
            return (
                <div className="max-w-[200px] rounded-lg overflow-hidden border border-gray-200 shadow-sm cursor-pointer" onClick={() => imgUrl && window.open(imgUrl, '_blank')}>
                    {imgUrl ? <img src={imgUrl} alt="Image" className="w-full h-auto" /> : <div className="p-4 bg-gray-100 text-xs">图片加载中...</div>}
                </div>
            );
        } else if (type === 'TIMFileElem' || type === 'TIMFile') {
            return (
                <div className="flex items-center gap-3 p-3 bg-white rounded-lg border border-gray-200 shadow-sm min-w-[200px] cursor-pointer hover:bg-gray-50" onClick={() => payload?.fileUrl && window.open(payload.fileUrl, '_blank')}>
                    <div className="w-10 h-10 bg-blue-100 rounded-lg flex items-center justify-center text-blue-600">
                        <Paperclip size={20} />
                    </div>
                    <div className="flex flex-col overflow-hidden">
                        <span className="text-sm font-medium text-gray-800 truncate">{payload?.fileName || '未知文件'}</span>
                        <span className="text-[10px] text-gray-500">{payload?.fileSize ? (payload.fileSize / 1024).toFixed(1) + ' KB' : ''}</span>
                    </div>
                </div>
            );
        }

        console.warn('Unknown message type:', msg);
        return (
            <div className="p-2 bg-gray-100 text-gray-500 text-xs rounded border border-gray-200">
                [不支持的消息类型: {type || JSON.stringify(msg)}]
            </div>
        );
    };

    const handleMinimize = () => getCurrentWindow().minimize();
    const handleMaximize = async () => {
        const win = getCurrentWindow();
        if (await win.isMaximized()) {
            await win.unmaximize();
        } else {
            await win.maximize();
        }
    };
    const handleClose = () => getCurrentWindow().close();

    return (
        <div className="h-screen w-screen bg-[#f3f9fe] flex flex-col overflow-hidden border border-gray-300 selection:bg-blue-100 selection:text-blue-900 shadow-xl">
            {/* Title Bar - Enhanced */}
            <div data-tauri-drag-region className="h-12 bg-gradient-to-r from-blue-50 to-white flex items-center justify-between px-4 border-b border-gray-200 select-none">
                <div className="flex items-center gap-3">
                    <div className="bg-blue-100 p-1.5 rounded-lg text-blue-600"><MessageCircle size={18} /></div>
                    <div className="flex flex-col">
                        <span className="font-bold text-gray-800 text-sm">班级群 - {groupclassId}</span>
                        <span className="text-[10px] text-gray-500 flex items-center gap-1">
                            {!isReady ? '连接中...' : `在线人数: ${members.length}`}
                        </span>
                    </div>
                </div>
                <div className="flex items-center gap-1">
                    {/* Data Import Trigger */}
                    <button
                        onClick={() => setShowCustomList(true)}
                        className="p-1.5 hover:bg-gray-200 rounded-md text-gray-500"
                        title="数据导入"
                    >
                        <Paperclip size={16} />
                    </button>

                    {/* Group Settings Menu */}
                    <div className="relative" ref={groupMenuRef}>
                        <button
                            onClick={() => setShowGroupMenu(!showGroupMenu)}
                            className="p-1.5 hover:bg-gray-200 rounded-md text-gray-500"
                            title="群设置"
                        >
                            <MoreHorizontal size={16} />
                        </button>

                        {showGroupMenu && (
                            <div className="absolute right-0 top-8 w-40 bg-white rounded-lg shadow-xl border border-gray-200 py-1 z-50 animate-in fade-in slide-in-from-top-2 duration-150">
                                <button
                                    onClick={handleExitGroup}
                                    className="w-full px-4 py-2 text-left text-sm text-gray-700 hover:bg-gray-100 flex items-center gap-2"
                                >
                                    <LogOut size={14} />
                                    <span>退出群聊</span>
                                </button>
                                {isOwner && (
                                    <button
                                        onClick={handleDisbandGroup}
                                        className="w-full px-4 py-2 text-left text-sm text-red-600 hover:bg-red-50 flex items-center gap-2"
                                    >
                                        <Trash2 size={14} />
                                        <span>解散群聊</span>
                                    </button>
                                )}
                            </div>
                        )}
                    </div>

                    <button onClick={handleMinimize} className="p-1.5 hover:bg-gray-200 rounded-md text-gray-500"><Minus size={16} /></button>
                    <button onClick={handleMaximize} className="p-1.5 hover:bg-gray-200 rounded-md text-gray-500">{isMaximized ? <Copy size={16} /> : <Square size={16} />}</button>
                    <button onClick={handleClose} className="p-1.5 hover:bg-red-500 hover:text-white rounded-md text-gray-500"><X size={16} /></button>
                </div>
            </div>

            <div className="flex-1 flex overflow-hidden">
                {/* Main Chat Area */}
                <div className="flex-1 flex flex-col min-w-0 bg-[#f5f6f7]">
                    {/* Messages */}
                    <div className="flex-1 overflow-y-auto p-4 space-y-4 custom-scrollbar">
                        {messages.map((msg, index) => {
                            const isSelf = msg.flow === 'out';
                            return (
                                <div key={msg.ID || index} className={`flex gap-3 ${isSelf ? 'flex-row-reverse' : ''}`}>
                                    <div className={`w-9 h-9 rounded-full flex items-center justify-center text-xs font-bold border shadow-sm shrink-0 select-none ${isSelf ? 'bg-blue-600 text-white' : 'bg-white text-gray-600 border-gray-200'}`}>
                                        {isSelf ? '我' : (msg.nick || msg.from || '?').substring(0, 1).toUpperCase()}
                                    </div>
                                    <div className={`flex flex-col gap-1 max-w-[70%] ${isSelf ? 'items-end' : ''}`}>
                                        <div className="flex items-center gap-2">
                                            <span className="text-xs text-gray-400">{isSelf ? '' : (msg.nick || msg.from)}</span>
                                            <span className="text-[10px] text-gray-300">{new Date(msg.time * 1000).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}</span>
                                        </div>
                                        {renderMessageContent(msg)}
                                    </div>
                                </div>
                            );
                        })}
                        {messages.length === 0 && (
                            <div className="text-center text-gray-400 mt-10 text-sm">
                                {isReady ? '暂无消息' : '正在加载聊天...'}
                            </div>
                        )}
                    </div>

                    {/* Input ToolBar */}
                    <div className="h-[160px] bg-white border-t border-gray-200 flex flex-col">
                        <div className="flex items-center gap-2 p-2 px-4">
                            <input type="file" ref={imageInputRef} className="hidden" accept="image/*" onChange={handleImageSelect} />
                            <input type="file" ref={fileInputRef} className="hidden" onChange={handleFileSelect} />

                            <button className="text-gray-500 hover:text-gray-700 p-1"><Smile size={20} /></button>
                            <button className="text-gray-500 hover:text-gray-700 p-1" onClick={() => imageInputRef.current?.click()} title="发送图片"><Image size={20} /></button>
                            <button className="text-gray-500 hover:text-gray-700 p-1" onClick={() => fileInputRef.current?.click()} title="发送文件"><Paperclip size={20} /></button>
                            <button className="text-gray-500 hover:text-gray-700 p-1"><Clock size={20} /></button>
                        </div>
                        <textarea
                            value={inputText}
                            onChange={(e) => setInputText(e.target.value)}
                            onKeyDown={handleKeyDown}
                            className="flex-1 resize-none px-4 py-2 outline-none text-gray-800 text-sm leading-relaxed custom-scrollbar"
                            placeholder=""
                        ></textarea>
                        <div className="p-2 px-4 flex justify-end">
                            <button
                                onClick={handleSendMessage}
                                disabled={!isReady}
                                className={`px-6 py-1.5 rounded text-sm transition-colors border ${isReady ? 'bg-blue-50 text-blue-600 border-blue-200 hover:bg-blue-100' : 'bg-gray-100 text-gray-400 border-gray-200 cursor-not-allowed'}`}
                            >
                                发送 (S)
                            </button>
                        </div>
                    </div>
                </div>

                {/* Right Sidebar - Member List */}
                <div className="w-48 bg-white border-l border-gray-200 flex flex-col">
                    <div className="h-10 border-b border-gray-100 flex items-center px-4 text-xs text-gray-500 font-medium">
                        群成员 ({members.length})
                    </div>

                    <div className="flex-1 overflow-y-auto p-2 space-y-1 custom-scrollbar">
                        {members.map((member) => (
                            <div key={member.userID} className="flex items-center gap-2 p-1.5 hover:bg-gray-50 rounded cursor-pointer">
                                <div className="w-6 h-6 rounded-full bg-gray-100 flex items-center justify-center text-[10px] text-gray-500 border border-gray-200">
                                    {member.avatar ? <img src={member.avatar} className="w-full h-full rounded-full" /> : (member.nick || member.userID).substring(0, 1).toUpperCase()}
                                </div>
                                <span className="text-xs text-gray-700 truncate">{member.nick || member.userID}</span>
                            </div>
                        ))}
                    </div>
                </div>
            </div>

            {/* Modals */}
            <CustomListModal
                isOpen={showCustomList}
                onClose={() => setShowCustomList(false)}
                classId={groupclassId}
            />

            {/* Transfer Ownership Modal (for owner exit) */}
            {showTransferModal && (
                <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/50 backdrop-blur-sm">
                    <div className="bg-white rounded-xl shadow-2xl w-[400px] p-6">
                        <h2 className="text-lg font-bold text-gray-800 mb-4 flex items-center gap-2">
                            <UserCheck size={20} className="text-blue-500" />
                            选择新群主
                        </h2>
                        <p className="text-sm text-gray-500 mb-4">您是群主，退出前需先转让群主身份给其他成员。</p>
                        <div className="max-h-[250px] overflow-y-auto space-y-2 mb-4">
                            {members.filter(m => m.userID !== localStorage.getItem('teacher_unique_id')).map((member) => (
                                <button
                                    key={member.userID}
                                    onClick={() => handleTransferAndQuit(member.userID)}
                                    className="w-full flex items-center gap-3 p-3 rounded-lg hover:bg-blue-50 border border-gray-200 transition-colors text-left"
                                >
                                    <div className="w-8 h-8 rounded-full bg-gray-100 flex items-center justify-center text-xs text-gray-500 border border-gray-200">
                                        {member.avatar ? <img src={member.avatar} className="w-full h-full rounded-full" /> : (member.nick || member.userID).substring(0, 1).toUpperCase()}
                                    </div>
                                    <span className="text-sm text-gray-700">{member.nick || member.userID}</span>
                                    {member.role === 'Admin' && <span className="text-[10px] bg-blue-100 text-blue-600 px-1.5 py-0.5 rounded">管理员</span>}
                                </button>
                            ))}
                        </div>
                        <button
                            onClick={() => setShowTransferModal(false)}
                            className="w-full py-2 text-sm text-gray-500 hover:bg-gray-100 rounded-lg transition-colors"
                        >
                            取消
                        </button>
                    </div>
                </div>
            )}
        </div>
    );
};

export default ClassChatWindow;
