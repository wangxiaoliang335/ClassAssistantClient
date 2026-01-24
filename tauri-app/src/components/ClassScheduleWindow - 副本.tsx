import { useEffect, useState } from 'react';
import { useParams } from 'react-router-dom';
import { getCurrentWindow } from '@tauri-apps/api/window';
import { invoke } from '@tauri-apps/api/core';
import { Minus, X, Square, Copy, MessageCircle, Calendar, Users, BookOpen, Shuffle, Clock, Grid, LayoutDashboard, Layers, Award, Mic, FileSpreadsheet, BarChart2, ArrowUpDown, Bell } from 'lucide-react';
import { getTIMGroups, isSDKReady, loginTIM } from '../utils/tim';
import { sendMessageWS } from '../utils/websocket';
import RandomCallModal from './modals/RandomCallModal';
import HomeworkModal from './modals/HomeworkModal';
import CountdownModal from './modals/CountdownModal';
import CourseScheduleModal from './modals/CourseScheduleModal';
import DutyRosterModal from './modals/DutyRosterModal';
import GroupScoreModal from './modals/GroupScoreModal';
import ClassInfoModal from './modals/ClassInfoModal';
import CustomListModal from './modals/CustomListModal';
import NotificationModal from './modals/NotificationModal';
import StudentImportModal from './modals/StudentImportModal';
import ScoreAnalysisModal from './modals/ScoreAnalysisModal';
import ArrangeSeatModal from './modals/ArrangeSeatModal';
import PrepareClassModal from './modals/PrepareClassModal';
import PostEvaluationModal from './modals/PostEvaluationModal';
import NotificationCenterModal, { NotificationItem } from './modals/NotificationCenterModal';
import DailySchedule from './DailySchedule';
import SeatMap from './SeatMap';



const ClassScheduleWindow = () => {
    const { groupclassId } = useParams();
    const [isMaximized, setIsMaximized] = useState(false);
    const [isClassLogin, setIsClassLogin] = useState(false);

    // View State
    const [currentView, setCurrentView] = useState<'overview' | 'seatmap'>('overview');
    const [heatmapMode, setHeatmapMode] = useState<'none' | 'score' | 'attendance'>('none');

    // Data State
    const [className, setClassName] = useState("");
    const [studentCount, setStudentCount] = useState(0);

    // Modal State
    const [isRandomCallOpen, setIsRandomCallOpen] = useState(false);
    const [isHomeworkOpen, setIsHomeworkOpen] = useState(false);
    const [linkHomeworkEnabled, setLinkHomeworkEnabled] = useState(false);
    const [isCountdownOpen, setIsCountdownOpen] = useState(false);
    const [isDutyRosterOpen, setIsDutyRosterOpen] = useState(false);
    const [isGroupScoreOpen, setIsGroupScoreOpen] = useState(false);
    const [isCourseScheduleOpen, setIsCourseScheduleOpen] = useState(false);
    const [isClassInfoOpen, setIsClassInfoOpen] = useState(false);
    const [isCustomListOpen, setIsCustomListOpen] = useState(false);
    const [isNotificationOpen, setIsNotificationOpen] = useState(false);
    const [isNotificationCenterOpen, setIsNotificationCenterOpen] = useState(false); // New Notification Center
    const [notifications, setNotifications] = useState<NotificationItem[]>([]); // Data for Notification Center
    const [isStudentImportOpen, setIsStudentImportOpen] = useState(false);
    const [isScoreAnalysisOpen, setIsScoreAnalysisOpen] = useState(false);
    const [isArrangeSeatOpen, setIsArrangeSeatOpen] = useState(false);
    const [seatColorMap, setSeatColorMap] = useState<Record<string, string>>({});
    const [seatAnalysisMode, setSeatAnalysisMode] = useState<'none' | 'segment' | 'gradient'>('none');
    const [seatMapKey, setSeatMapKey] = useState(0);

    // PrepareClass / PostEvaluation Modal State
    const [isPrepareClassOpen, setIsPrepareClassOpen] = useState(false);
    const [isPostEvaluationOpen, setIsPostEvaluationOpen] = useState(false);
    const [selectedSubject, setSelectedSubject] = useState("");
    const [selectedTime, setSelectedTime] = useState("");

    const [backgroundImage, setBackgroundImage] = useState<string | null>(null);

    // Fetch Wallpaper Logic
    const fetchWallpaper = async () => {
        if (!groupclassId) return;
        try {
            // 1. Check Weekly Config First
            const weeklyResStr = await invoke<string>('fetch_weekly_config', { groupId: groupclassId });
            const weeklyRes = JSON.parse(weeklyResStr);

            if (weeklyRes.code === 200 && weeklyRes.data && weeklyRes.data.is_enabled && weeklyRes.data.weekly_wallpapers) {
                // Determine today's key (Backend: 0=Mon ... 6=Sun)
                const jsDay = new Date().getDay(); // 0=Sun, 1=Mon...
                const backendKey = jsDay === 0 ? "6" : String(jsDay - 1);

                const url = weeklyRes.data.weekly_wallpapers[backendKey];
                if (url) {
                    console.log("[ClassSchedule] Using Weekly Wallpaper for day:", backendKey, url);
                    setBackgroundImage(url);
                    return; // Priority to weekly
                }
            }

            // 2. Fallback to Current Class Wallpaper
            const wallpapersResStr = await invoke<string>('fetch_class_wallpapers', { groupId: groupclassId });
            const wallpapersRes = JSON.parse(wallpapersResStr);

            if (wallpapersRes.code === 200 && wallpapersRes.data && wallpapersRes.data.wallpapers) {
                const current = wallpapersRes.data.wallpapers.find((wp: any) => wp.is_current === 1);
                if (current) {
                    console.log("[ClassSchedule] Using Current Class Wallpaper:", current.image_url);
                    setBackgroundImage(current.image_url);
                } else {
                    setBackgroundImage(null);
                    console.log("[ClassSchedule] No current wallpaper set");
                }
            }
        } catch (e) {
            console.error("[ClassSchedule] Failed to fetch wallpaper:", e);
        }
    };

    useEffect(() => {
        const userInfoStr = localStorage.getItem('user_info');
        if (userInfoStr) {
            try {
                const u = JSON.parse(userInfoStr);
                setIsClassLogin(u.loginType === 'class');
            } catch {
                setIsClassLogin(false);
            }
        }
        const saved = groupclassId ? localStorage.getItem(`class_link_homework_${groupclassId}`) : null;
        if (saved !== null) {
            setLinkHomeworkEnabled(saved === 'true');
        }

        const handleHomeworkLinkChanged = (event: Event) => {
            const detail = (event as CustomEvent).detail;
            if (detail?.linkHomework !== undefined) {
                setLinkHomeworkEnabled(!!detail.linkHomework);
            }
        };
        window.addEventListener('homework-link-changed', handleHomeworkLinkChanged as EventListener);

        const checkMaximized = async () => {
            const win = getCurrentWindow();
            setIsMaximized(await win.isMaximized());
        };
        checkMaximized();

        const unlisten = getCurrentWindow().onResized(async () => {
            const win = getCurrentWindow();
            setIsMaximized(await win.isMaximized());
        });

        // Fetch Class Info
        fetchClassInfo();
        // Fetch Wallpaper
        fetchWallpaper();

        return () => {
            unlisten.then(f => f());
            window.removeEventListener('homework-link-changed', handleHomeworkLinkChanged as EventListener);
        }
    }, [groupclassId]);

    const handleOpenHomework = () => {
        if (!linkHomeworkEnabled) {
            alert('请先在班级信息中开启“关联家庭作业”');
            return;
        }
        setIsHomeworkOpen(true);
    };

    // Listen for WebSocket Messages (Notification Center)
    useEffect(() => {
        const handleWSMessage = (event: CustomEvent) => {
            try {
                const msg = JSON.parse(event.detail);
                if (msg.type === "unread_notifications") {
                    console.log("[ClassSchedule] Received Notifications Payload:", msg.data);

                    // msg.data is an array of objects. We need to cast them to NotificationItem
                    const newItems = (msg.data || []) as NotificationItem[];

                    setNotifications(prev => {
                        // Create a map of existing items by ID for easy lookup
                        const existingMap = new Map(prev.map(item => [item.id, item]));

                        // Merge/Overwrite with new items
                        newItems.forEach(item => {
                            existingMap.set(item.id, item);
                        });

                        // Convert back to array
                        return Array.from(existingMap.values());
                    });

                    // Auto-open if important? Maybe just show badge.
                } else if (msg.type === "monitor") {
                    console.log("[ClassSchedule] Received Monitor Command:", msg);
                    const action = msg.action;
                    const senderId = msg.sender_id;
                    const ts = Math.floor(Date.now() / 1000);
                    // Use the group_id from the received message (from teacher), fallback to groupclassId
                    const targetGroupId = msg.group_id || groupclassId;

                    if (action === "start_stream") {
                        console.log("[ClassSchedule] Starting Remote Monitoring...");

                        // Construct Stream Name & Pull URL
                        // StreamID format: #!::r=live/CLASSID_TS,m=publish
                        const streamName = `live/${targetGroupId}_${ts}`;
                        const publishUrl = `srt://47.100.126.194:10080?streamid=#!::r=${streamName},m=publish`;
                        const playUrl = `webrtc://47.100.126.194/live/${targetGroupId}_${ts}`;

                        // Call Rust Backend to Start Streaming
                        invoke('start_stream', { pullUrl: publishUrl })
                            .then(() => console.log("[ClassSchedule] Streaming started successfully"))
                            .catch(err => console.error("[ClassSchedule] Failed to start stream:", err));

                        const response = {
                            type: "camera_stream",
                            action: "start_pull",
                            class_id: groupclassId,
                            group_id: targetGroupId,  // Use the same group_id from teacher's request
                            stream_name: streamName,
                            pull_url: playUrl,
                            sender_id: senderId,
                            sender_name: "Class Terminal",
                            ts: ts
                        };

                        // Try to get current user ID
                        try {
                            const uStr = localStorage.getItem('user_info');
                            if (uStr) {
                                const u = JSON.parse(uStr);
                                if (u.user_id) response.sender_id = u.user_id;
                                if (u.name) response.sender_name = u.name;
                            }
                        } catch (e) { }

                        console.log("[ClassSchedule] Sending start_pull feedback:", response);
                        const wsPayload = JSON.stringify(response);
                        const wsMessage = wsPayload.startsWith('to:') ? wsPayload : `to:${targetGroupId}:${wsPayload}`;
                        sendMessageWS(wsMessage);

                    } else if (action === "stop_stream") {
                        console.log("[ClassSchedule] Stopping Remote Monitoring...");

                        // Call Rust Backend to Stop Streaming
                        invoke('stop_stream')
                            .then(() => console.log("[ClassSchedule] Streaming stopped successfully"))
                            .catch(err => console.error("[ClassSchedule] Failed to stop stream:", err));

                        const response = {
                            type: "camera_stream",
                            action: "stop_pull",
                            class_id: groupclassId,
                            group_id: targetGroupId,  // Use the same group_id from teacher's request
                            sender_id: "",
                            ts: ts
                        };
                        try {
                            const uStr = localStorage.getItem('user_info');
                            if (uStr) {
                                const u = JSON.parse(uStr);
                                if (u.user_id) response.sender_id = u.user_id;
                            }
                        } catch (e) { }

                        console.log("[ClassSchedule] Sending stop_pull feedback:", response);
                        const wsPayload = JSON.stringify(response);
                        const wsMessage = wsPayload.startsWith('to:') ? wsPayload : `to:${targetGroupId}:${wsPayload}`;
                        sendMessageWS(wsMessage);
                    }
                }
            } catch (e) {
                console.error("[ClassSchedule] Error parsing WS message:", e);
            }
        };

        window.addEventListener('ws-message', handleWSMessage as EventListener);
        return () => {
            window.removeEventListener('ws-message', handleWSMessage as EventListener);
        };
    }, []);

    const fetchClassInfo = async () => {
        if (!groupclassId) return;

        // Initial fallback
        setClassName(prev => prev || groupclassId || "未知班级");

        try {
            const userInfoStr = localStorage.getItem('user_info');
            let token = "";
            let idCard = "";
            let teacherId = "";

            if (userInfoStr) {
                try {
                    const u = JSON.parse(userInfoStr);
                    token = u.token || "";
                    idCard = u.id_number || u.data?.id_number || u.strIdNumber || localStorage.getItem('id_number') || "";
                    teacherId = u.teacher_unique_id || u.data?.teacher_unique_id || localStorage.getItem('teacher_unique_id') || "";
                } catch (e) { }
            }

            console.log(`[ClassSchedule] Fetching Info for ID: ${groupclassId}, Teacher: ${teacherId}`);

            // Parallel requests to try to find the name
            const p1 = invoke<string>('get_group_members', { groupId: groupclassId, token })
                .then(resStr => {
                    const res = JSON.parse(resStr);
                    if (res.code === 200 || (res.data && res.data.code === 200)) {
                        const data = res.data || {};
                        const list = data.members || [];
                        setStudentCount(list.length || data.group_info?.MemberNum || 0);
                        const name = data.group_info?.Name || data.group_info?.GroupName;
                        console.log("[ClassSchedule] p1 (Members) found:", name);
                        return name;
                    }
                    return null;
                })
                .catch(e => { console.error("[ClassSchedule] p1 error:", e); return null; });

            const p2 = idCard ? invoke<string>('get_user_friends', { idCard, token })
                .then(resStr => {
                    const res = JSON.parse(resStr);
                    if (res.data) {
                        const allGroups = [...(res.data.owner_groups || []), ...(res.data.member_groups || [])];
                        // Fuzzy match because string ID from URL vs maybe formatted ID from server
                        const target = allGroups.find((g: any) => g.group_id == groupclassId);
                        console.log("[ClassSchedule] p2 (Friends) found:", target?.group_name);
                        return target?.group_name;
                    }
                    return null;
                })
                .catch(e => { console.error("[ClassSchedule] p2 error:", e); return null; }) : Promise.resolve(null);

            const p3 = teacherId ? invoke<string>('get_teacher_classes', { teacherUniqueId: teacherId, token })
                .then(resStr => {
                    const res = JSON.parse(resStr);
                    const classes = res.data?.classes || ((Array.isArray(res.data)) ? res.data : []);
                    const target = classes.find((c: any) => c.class_code == groupclassId);
                    console.log("[ClassSchedule] p3 (Classes) found:", target?.class_name);
                    return target?.class_name;
                })
                .catch(e => { console.error("[ClassSchedule] p3 error:", e); return null; }) : Promise.resolve(null);

            // 4. TIM SDK Groups (Most accurate for "Group Chats")
            const p4 = (async () => {
                if (!teacherId) return null;
                try {
                    // We must login because this is a new window/context
                    let ready = isSDKReady;
                    if (!ready) {
                        console.log("[ClassSchedule] TIM Login required. Fetching Sig...");
                        const userSig = await invoke<string>('get_user_sig', { userId: teacherId });
                        if (userSig) {
                            ready = await loginTIM(teacherId, userSig) as boolean;
                            console.log("[ClassSchedule] TIM Login Result:", ready);
                        }
                    }

                    if (ready) {
                        const groups = await getTIMGroups();
                        console.log(`[ClassSchedule] TIM Groups fetched: ${groups.length}`);
                        const target = groups.find((g: any) => g.groupID == groupclassId);
                        if (target) {
                            console.log("[ClassSchedule] TIM Match Found:", target.name, target.groupID);
                        } else {
                            console.log("[ClassSchedule] TIM No Match for:", groupclassId);
                        }
                        return target?.name;
                    } else {
                        console.warn("[ClassSchedule] TIM SDK not ready after login attempt");
                    }
                } catch (e) {
                    console.error("[ClassSchedule] TIM Fetch failed in Window:", e);
                }
                return null;
            })();

            // Wait for results and pick the best name
            const [nameFromGroup, nameFromFriends, nameFromClasses, nameFromTIM] = await Promise.all([p1, p2, p3, p4]);

            console.log("[ClassSchedule] Final Candidates:", { nameFromGroup, nameFromFriends, nameFromClasses, nameFromTIM });

            // Priority: TIM Name > Server Group Name > Teacher Classes Name > Group Info Name
            const finalName = nameFromTIM || nameFromFriends || nameFromClasses || nameFromGroup;
            if (finalName) {
                setClassName(finalName);
            } else {
                // If we still found nothing, keep showing ID or default, but remove "Loading..." logic if any
                setClassName(prev => prev || groupclassId || "未命名班级");
            }

        } catch (e) {
            console.error("Failed to fetch class info:", e);
            setClassName(prev => prev || groupclassId || "网络错误");
        }
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

    const handleOpenChat = () => {
        invoke('open_chat_window', { groupclassId });
    };

    const handleOpenIntercom = () => {
        if (groupclassId) {
            invoke('open_intercom_window', { groupId: groupclassId });
        }
    };

    // Note: handlePublishHomework removed - HomeworkModal now handles WebSocket messaging internally

    return (
        <div
            className="h-screen w-screen bg-[#f8fbff] flex flex-col overflow-hidden border border-gray-300 select-none text-gray-700 font-sans relative transition-all duration-500 bg-cover bg-center"
            style={backgroundImage ? { backgroundImage: `url(${backgroundImage})` } : {}}
        >
            {/* Overlay to ensure text readability if background is set */}
            {backgroundImage && (
                <div className="absolute inset-0 bg-white/75 backdrop-blur-sm z-0 pointer-events-none"></div>
            )}

            {/* Title Bar - Added z-10 for layering */}
            <div data-tauri-drag-region className="h-10 bg-white/50 backdrop-blur-md flex items-center justify-between px-4 border-b border-gray-200/50 z-50 relative">
                <div className="flex items-center gap-4">
                    <div className="font-bold text-gray-700 flex items-center gap-2">
                        <div className="bg-blue-500 text-white p-1 rounded-md shadow-sm shadow-blue-200"><Users size={12} /></div>
                        <span className="text-sm tracking-wide">班级空间 - {className || groupclassId}</span>
                    </div>
                    {/* View Switcher */}
                    <div className="flex bg-gray-100/80 p-0.5 rounded-lg border border-gray-200/50">
                        <button
                            onClick={() => setCurrentView('overview')}
                            className={`px-2 py-0.5 text-xs rounded-md flex items-center gap-1 transition-all ${currentView === 'overview' ? 'bg-white text-blue-600 shadow-sm' : 'text-gray-500 hover:text-gray-700'}`}
                        >
                            <LayoutDashboard size={12} /> 概览
                        </button>
                        <button
                            onClick={() => setCurrentView('seatmap')}
                            className={`px-2 py-0.5 text-xs rounded-md flex items-center gap-1 transition-all ${currentView === 'seatmap' ? 'bg-white text-blue-600 shadow-sm' : 'text-gray-500 hover:text-gray-700'}`}
                        >
                            <Grid size={12} /> 座位表
                        </button>
                    </div>
                </div>
                <div className="flex items-center gap-1.5 opacity-80 hover:opacity-100 transition-opacity">
                    {/* Notification Bell */}
                    <button
                        onClick={() => setIsNotificationCenterOpen(true)}
                        className="p-1 hover:bg-gray-200 rounded text-gray-500 mr-2 relative"
                    >
                        <Bell size={16} />
                        {notifications.length > 0 && notifications.some(n => n.is_read === 0) && (
                            <span className="absolute top-0.5 right-0.5 w-2 h-2 bg-red-500 rounded-full border border-white"></span>
                        )}
                    </button>

                    <button onClick={() => setIsClassInfoOpen(true)} className="px-2 py-0.5 mr-2 rounded text-gray-500 hover:bg-gray-200 font-bold">...</button>
                    <button onClick={handleMinimize} className="p-1 hover:bg-gray-200 rounded text-gray-500"><Minus size={14} /></button>
                    <button onClick={handleMaximize} className="p-1 hover:bg-gray-200 rounded text-gray-500">{isMaximized ? <Copy size={14} /> : <Square size={14} />}</button>
                    <button onClick={handleClose} className="p-1 hover:bg-red-500 hover:text-white rounded text-gray-500"><X size={14} /></button>
                </div>
            </div>

            {/* Main Content Area - Added z-10 for layering */}
            <div className="flex-1 overflow-hidden p-4 md:p-6 relative flex flex-col gap-4 z-10">
                {currentView === 'overview' ? (
                    <div className="flex-1 flex flex-col gap-4 h-full">
                        {/* Top Row - Daily Schedule Timeline (Full Width) */}
                        <div className="bg-white rounded-2xl p-4 border border-gray-100 shadow-sm flex-shrink-0">
                            <div className="flex items-center justify-between mb-3">
                                <div className="flex items-center gap-2">
                                    <span className="w-1 h-5 bg-blue-500 rounded-full"></span>
                                    <h3 className="font-bold text-gray-800">今日课程</h3>
                                </div>
                                <button
                                    onClick={() => setIsCourseScheduleOpen(true)}
                                    className="text-xs text-blue-600 bg-blue-50 hover:bg-blue-100 px-3 py-1.5 rounded-full font-medium transition-colors"
                                >
                                    查看完整课表
                                </button>
                            </div>
                            <DailySchedule
                                classId={groupclassId}
                                onPrepareClass={(subject, time) => {
                                    setSelectedSubject(subject);
                                    setSelectedTime(time);
                                    setIsPrepareClassOpen(true);
                                }}
                                onPostEvaluation={(subject) => {
                                    setSelectedSubject(subject);
                                    setIsPostEvaluationOpen(true);
                                }}
                            />
                        </div>

                        {/* Bottom Row - 3 Column Grid */}
                        <div className="flex-1 grid grid-cols-12 gap-4 min-h-0">
                            {/* Left Column - Class Info & Notifications */}
                            <div className="col-span-3 flex flex-col gap-4">
                                {/* Hero Card */}
                                <div className="bg-white p-5 rounded-2xl border border-gray-100 shadow-sm relative overflow-hidden">
                                    <div className="absolute top-0 right-0 w-24 h-24 bg-gradient-to-br from-blue-50 to-indigo-50 rounded-bl-full -mr-6 -mt-6 opacity-50"></div>
                                    <div className="relative z-10">
                                        <h1 className="text-lg font-bold text-gray-800 mb-0.5">{className}</h1>
                                        <div className="mt-3 flex items-center gap-3 text-xs text-gray-400">
                                            <span className="flex items-center gap-1"><Users size={12} /> {studentCount} 人</span>
                                            <span className="w-px h-3 bg-gray-200"></span>
                                            <span className="flex items-center gap-1"><Calendar size={12} /> --</span>
                                        </div>
                                    </div>
                                </div>

                                {/* Notifications Card */}
                                <div
                                    onClick={() => setIsNotificationOpen(true)}
                                    className="flex-1 bg-gradient-to-br from-indigo-500 to-purple-600 rounded-2xl p-4 text-white shadow-lg shadow-purple-200/50 flex flex-col cursor-pointer transition-transform hover:-translate-y-0.5"
                                >
                                    <div className="flex items-center justify-between mb-2">
                                        <h3 className="font-bold text-sm opacity-90 flex items-center gap-2">
                                            <span className="bg-white/20 p-1 rounded"><MessageCircle size={12} /></span>
                                            班级通知
                                        </h3>
                                    </div>
                                    <p className="text-xs opacity-80 leading-relaxed line-clamp-3 flex-1">
                                        暂无通知
                                    </p>
                                    <button className="mt-2 text-xs bg-white/20 hover:bg-white/30 py-1.5 rounded-lg transition-colors font-medium">
                                        查看全部
                                    </button>
                                </div>
                            </div>

                            {/* Middle Column - Homework & Stats */}
                            <div className="col-span-5 flex flex-col gap-4">
                                {/* Homework Card */}
                                <div
                                    onClick={handleOpenHomework}
                                    className="bg-white rounded-2xl p-5 border border-gray-100 shadow-sm flex items-center justify-between hover:border-green-200 transition-all cursor-pointer"
                                >
                                    <div className="flex items-center gap-4">
                                        <div className="w-12 h-12 bg-green-50 text-green-600 rounded-xl flex items-center justify-center">
                                            <BookOpen size={24} />
                                        </div>
                                        <div>
                                            <h4 className="font-bold text-gray-800">作业批改</h4>
                                            <p className="text-xs text-gray-500 mt-0.5">暂无数据</p>
                                        </div>
                                    </div>
                                    <div className="flex items-end gap-1">
                                        <span className="text-2xl font-bold text-gray-800">--</span>
                                        <span className="text-xs text-gray-400 mb-1.5">/ --</span>
                                    </div>
                                </div>

                                {/* Quick Stats */}
                                <div className="flex-1 grid grid-cols-3 gap-3">
                                    <div className="bg-white rounded-xl p-4 border border-gray-100 flex flex-col items-center justify-center">
                                        <span className="text-2xl font-bold text-blue-600">--</span>
                                        <span className="text-xs text-gray-400 mt-1">出勤率</span>
                                    </div>
                                    <div className="bg-white rounded-xl p-4 border border-gray-100 flex flex-col items-center justify-center">
                                        <span className="text-2xl font-bold text-green-600">--</span>
                                        <span className="text-xs text-gray-400 mt-1">平均分</span>
                                    </div>
                                    <div className="bg-white rounded-xl p-4 border border-gray-100 flex flex-col items-center justify-center">
                                        <span className="text-2xl font-bold text-purple-600">--</span>
                                        <span className="text-xs text-gray-400 mt-1">班级评级</span>
                                    </div>
                                </div>
                            </div>

                            {/* Right Column - Tools Grid */}
                            <div className="col-span-4 bg-white/50 rounded-2xl border border-gray-100 p-4 flex flex-col">
                                <h3 className="text-sm font-bold text-gray-600 mb-3 flex items-center gap-2">
                                    <Layers size={14} /> 快捷工具
                                </h3>
                                <div className="grid grid-cols-2 gap-2 flex-1 content-start">
                                    <button onClick={handleOpenIntercom} className="h-16 bg-white rounded-xl border border-gray-100 hover:shadow-sm hover:border-red-200 transition-all flex items-center gap-3 px-3 group">
                                        <div className="p-2 rounded-lg bg-red-50 text-red-500 group-hover:bg-red-100 transition-all">
                                            <Mic size={16} />
                                        </div>
                                        <span className="text-xs font-medium text-gray-600">语音对讲</span>
                                    </button>

                                    <button onClick={() => setIsCustomListOpen(true)} className="h-16 bg-white rounded-xl border border-gray-100 hover:shadow-sm hover:border-orange-200 transition-all flex items-center gap-3 px-3 group">
                                        <div className="p-2 rounded-lg bg-orange-50 text-orange-500 group-hover:bg-orange-100 transition-all">
                                            <FileSpreadsheet size={16} />
                                        </div>
                                        <span className="text-xs font-medium text-gray-600">更多导入</span>
                                    </button>

                                    <button onClick={() => setIsRandomCallOpen(true)} className="h-16 bg-white rounded-xl border border-gray-100 hover:shadow-sm hover:border-cyan-200 transition-all flex items-center gap-3 px-3 group">
                                        <div className="p-2 rounded-lg bg-cyan-50 text-cyan-500 group-hover:bg-cyan-100 transition-all">
                                            <Shuffle size={16} />
                                        </div>
                                        <span className="text-xs font-medium text-gray-600">随机点名</span>
                                    </button>

                                    <button onClick={() => setIsCountdownOpen(true)} className="h-16 bg-white rounded-xl border border-gray-100 hover:shadow-sm hover:border-rose-200 transition-all flex items-center gap-3 px-3 group">
                                        <div className="p-2 rounded-lg bg-rose-50 text-rose-500 group-hover:bg-rose-100 transition-all">
                                            <Clock size={16} />
                                        </div>
                                        <span className="text-xs font-medium text-gray-600">倒计时</span>
                                    </button>

                                    <button onClick={() => setIsGroupScoreOpen(true)} className="h-16 bg-white rounded-xl border border-gray-100 hover:shadow-sm hover:border-violet-200 transition-all flex items-center gap-3 px-3 group">
                                        <div className="p-2 rounded-lg bg-violet-50 text-violet-500 group-hover:bg-violet-100 transition-all">
                                            <Award size={16} />
                                        </div>
                                        <span className="text-xs font-medium text-gray-600">小组评价</span>
                                    </button>

                                    <button onClick={() => setIsStudentImportOpen(true)} className="h-16 bg-white rounded-xl border border-gray-100 hover:shadow-sm hover:border-blue-200 transition-all flex items-center gap-3 px-3 group">
                                        <div className="p-2 rounded-lg bg-blue-50 text-blue-500 group-hover:bg-blue-100 transition-all">
                                            <Grid size={16} />
                                        </div>
                                        <span className="text-xs font-medium text-gray-600">座位导入</span>
                                    </button>

                                    <button onClick={() => setIsScoreAnalysisOpen(true)} className="h-16 bg-white rounded-xl border border-gray-100 hover:shadow-sm hover:border-indigo-200 transition-all flex items-center gap-3 px-3 group">
                                        <div className="p-2 rounded-lg bg-indigo-50 text-indigo-500 group-hover:bg-indigo-100 transition-all">
                                            <BarChart2 size={16} />
                                        </div>
                                        <span className="text-xs font-medium text-gray-600">成绩分析</span>
                                    </button>

                                    <button onClick={() => setIsArrangeSeatOpen(true)} className="h-16 bg-white rounded-xl border border-gray-100 hover:shadow-sm hover:border-teal-200 transition-all flex items-center gap-3 px-3 group">
                                        <div className="p-2 rounded-lg bg-teal-50 text-teal-500 group-hover:bg-teal-100 transition-all">
                                            <ArrowUpDown size={16} />
                                        </div>
                                        <span className="text-xs font-medium text-gray-600">排座</span>
                                    </button>
                                </div>
                            </div>
                        </div>
                    </div>
                ) : (
                    <div className="h-full flex flex-col bg-white rounded-2xl border border-gray-100 shadow-sm overflow-hidden">
                        <div className="h-12 border-b border-gray-100 flex items-center justify-between px-4 bg-gray-50/50">
                            <h3 className="font-bold text-gray-700 flex items-center gap-2">
                                <Grid size={16} className="text-blue-500" /> 座位表
                            </h3>
                            <div className="flex items-center gap-2">
                                <span className="text-xs text-gray-500">热力图模式:</span>
                                <select
                                    className="bg-white border border-gray-200 text-xs rounded-md p-1 outline-none focus:border-blue-500 transition-colors"
                                    value={heatmapMode}
                                    onChange={(e) => setHeatmapMode(e.target.value as any)}
                                >
                                    <option value="none">无</option>
                                    <option value="score">成绩</option>
                                    <option value="attendance">考勤</option>
                                </select>
                            </div>
                        </div>
                        <div className="flex-1 overflow-hidden p-2">
                            <SeatMap classId={groupclassId} key={seatMapKey} colorMap={seatColorMap} isHeatmapMode={seatAnalysisMode === 'gradient'} />
                        </div>
                    </div>
                )}
            </div>

            {/* Bottom Dock - Glassmorphism */}
            <div className="h-20 flex items-center justify-center px-10 relative z-10">
                <div className="absolute inset-0 bg-white/80 backdrop-blur-lg border-t border-white/50 shadow-[0_-10px_40px_rgba(0,0,0,0.03)] z-0"></div>
                <div className="z-10 bg-white border border-gray-100 p-1.5 px-3 rounded-2xl flex items-center gap-6 shadow-lg shadow-gray-200/50">
                    <button
                        onClick={handleOpenChat}
                        className="flex flex-col items-center gap-1 group relative px-4"
                    >
                        <div className="relative">
                            <div className="w-10 h-10 bg-gradient-to-tr from-blue-500 to-blue-600 rounded-xl flex items-center justify-center text-white shadow-lg shadow-blue-200 group-hover:shadow-blue-300 group-hover:-translate-y-1 transition-all duration-300">
                                <MessageCircle size={20} />
                            </div>
                            <span className="absolute -top-1 -right-1 w-3 h-3 bg-red-500 border-2 border-white rounded-full"></span>
                        </div>
                        <span className="text-[10px] font-bold text-gray-500 group-hover:text-blue-600 transition-colors mt-0.5">进入群聊</span>
                    </button>

                    <div className="w-[1px] h-8 bg-gray-200"></div>

                    {/* Notice Button */}
                    <button
                        onClick={() => setIsNotificationOpen(true)}
                        className="flex flex-col items-center gap-1 group px-2"
                    >
                        <div className="w-9 h-9 bg-red-50 text-red-500 rounded-lg flex items-center justify-center group-hover:bg-red-100 group-hover:-translate-y-1 transition-all">
                            <Bell size={18} />
                        </div>
                        <span className="text-[10px] font-medium text-gray-500 group-hover:text-red-600">通知</span>
                    </button>

                    {/* Homework Button */}
                    <button
                        onClick={handleOpenHomework}
                        className="flex flex-col items-center gap-1 group px-2"
                    >
                        <div className="w-9 h-9 bg-green-50 text-green-500 rounded-lg flex items-center justify-center group-hover:bg-green-100 group-hover:-translate-y-1 transition-all">
                            <BookOpen size={18} />
                        </div>
                        <span className="text-[10px] font-medium text-gray-500 group-hover:text-green-600">作业</span>
                    </button>
                </div>
            </div>

            {/* Modals */}
            <RandomCallModal
                isOpen={isRandomCallOpen}
                onClose={() => setIsRandomCallOpen(false)}
                classId={groupclassId}
            />
            <HomeworkModal
                isOpen={isHomeworkOpen}
                onClose={() => setIsHomeworkOpen(false)}
                classId={groupclassId}
                groupId={groupclassId}
                groupName={className}
                readOnly={isClassLogin}
            />
            <CountdownModal
                isOpen={isCountdownOpen}
                onClose={() => setIsCountdownOpen(false)}
            />
            <DutyRosterModal
                isOpen={isDutyRosterOpen}
                onClose={() => setIsDutyRosterOpen(false)}
            />
            <GroupScoreModal
                isOpen={isGroupScoreOpen}
                onClose={() => setIsGroupScoreOpen(false)}
            />
            <CourseScheduleModal
                isOpen={isCourseScheduleOpen}
                onClose={() => setIsCourseScheduleOpen(false)}
                classId={groupclassId}
            />
            <ClassInfoModal
                isOpen={isClassInfoOpen}
                onClose={() => {
                    setIsClassInfoOpen(false);
                    fetchWallpaper(); // Refresh wallpaper when settings might have changed
                }}
                groupId={groupclassId}
                isClassGroup={true}
            />
            <CustomListModal
                isOpen={isCustomListOpen}
                onClose={() => setIsCustomListOpen(false)}
                classId={groupclassId}
            />
            <StudentImportModal
                isOpen={isStudentImportOpen}
                onClose={() => setIsStudentImportOpen(false)}
                classId={groupclassId || ""}
                onSuccess={() => setSeatMapKey(prev => prev + 1)}
            />
            <ScoreAnalysisModal
                isOpen={isScoreAnalysisOpen}
                onClose={() => setIsScoreAnalysisOpen(false)}
                classId={groupclassId || ""}
                onApply={(map, mode) => {
                    setSeatColorMap(map);
                    setSeatAnalysisMode(mode);
                    // Also switch to seat view if not already
                    if (currentView !== 'seatmap') setCurrentView('seatmap');
                }}
            />
            <NotificationModal
                isOpen={isNotificationOpen}
                onClose={() => setIsNotificationOpen(false)}
                classId={groupclassId}
                groupId={groupclassId}
                groupName={className}
                readOnly={isClassLogin}
            />
            <NotificationCenterModal
                isOpen={isNotificationCenterOpen}
                onClose={() => setIsNotificationCenterOpen(false)}
                notifications={notifications}
            />
            <ArrangeSeatModal
                isOpen={isArrangeSeatOpen}
                onClose={() => setIsArrangeSeatOpen(false)}
                classId={groupclassId}
                onArrange={async (arrangedStudents) => {
                    // Upload arranged seats to server
                    try {
                        const seatData = arrangedStudents.map((s, idx) => ({
                            student_id: s.id,
                            student_name: s.name,
                            row: Math.floor(idx / 11) + 1,
                            col: (idx % 11) + 1,
                        }));

                        await invoke('save_seat_arrangement', {
                            classId: groupclassId,
                            seatsJson: JSON.stringify(seatData)
                        });

                        // Refresh seat map
                        setSeatMapKey(prev => prev + 1);
                        setCurrentView('seatmap');
                        console.log('[Arrange] Seat arrangement saved successfully');
                    } catch (err) {
                        console.error('[Arrange] Failed to save seat arrangement:', err);
                        alert('保存座位表失败');
                    }
                }}
            />
            <PrepareClassModal
                isOpen={isPrepareClassOpen}
                onClose={() => setIsPrepareClassOpen(false)}
                subject={selectedSubject}
                time={selectedTime}
                classId={groupclassId}
                groupId={groupclassId}
            />
            <PostEvaluationModal
                isOpen={isPostEvaluationOpen}
                onClose={() => setIsPostEvaluationOpen(false)}
                subject={selectedSubject}
                classId={groupclassId}
                groupId={groupclassId}
            />
        </div>
    );
};

export default ClassScheduleWindow;
