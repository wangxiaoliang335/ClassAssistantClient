import { useState, useEffect } from 'react';
import Taskbar, { TaskbarItemType } from './Taskbar';
import { Minus, X, Square, Copy, LogOut } from 'lucide-react'; // Square for Maximize, Copy for Restore (simulated)
import { getCurrentWindow } from '@tauri-apps/api/window';
import TeacherSchedule from './TeacherSchedule';
import CreateClassGroupModal from './modals/CreateClassGroupModal';
import CreateNormalGroupModal from './modals/CreateNormalGroupModal';
import SearchAddModal from './modals/SearchAddModal';
import UserInfoModal from './modals/UserInfoModal';
import SchoolInfoModal from './modals/SchoolInfoModal';
import ClassTextMessageWindow from './ClassTextMessageWindow';
import { loginTIM, getTIMGroups } from '../utils/tim';
import { connectWS, sendMessageWS } from '../utils/websocket';

import { invoke } from '@tauri-apps/api/core';

interface DashboardProps {
    userInfo: any;
}

interface GroupInfo {
    group_id: string;
    group_name: string;
    face_url: string;
    is_class_group: boolean;
    classid?: string;
}

const Dashboard = ({ userInfo }: DashboardProps) => {
    const [activeApp, setActiveApp] = useState<TaskbarItemType | null>(null);
    const [isMaximized, setIsMaximized] = useState(false);
    const [currentUserInfo, setCurrentUserInfo] = useState(userInfo);
    const [classGroup, setClassGroup] = useState<GroupInfo | null>(null);
    const [loadingClassGroup, setLoadingClassGroup] = useState(false);

    // Modal States
    const [showCreateClassGroup, setShowCreateClassGroup] = useState(false);
    const [showCreateNormalGroup, setShowCreateNormalGroup] = useState(false);
    const [showSearchAdd, setShowSearchAdd] = useState(false);
    const [showUserInfo, setShowUserInfo] = useState(false);
    const [showSchoolInfo, setShowSchoolInfo] = useState(false);
    const [showTextMessageWindow, setShowTextMessageWindow] = useState(false);
    const [receiveNotification, setReceiveNotification] = useState(true);
    useEffect(() => {
        setCurrentUserInfo(userInfo);
    }, [userInfo]);

    useEffect(() => {
        const handleClassInfoUpdated = (event: Event) => {
            const detail = (event as CustomEvent).detail;
            if (!detail) return;
            setCurrentUserInfo((prev: any) => ({
                ...prev,
                ...detail,
                // Preserve loginType so class-mode UI doesn't disappear.
                loginType: prev?.loginType || detail?.loginType
            }));
        };

        window.addEventListener('class-info-updated', handleClassInfoUpdated as EventListener);
        return () => {
            window.removeEventListener('class-info-updated', handleClassInfoUpdated as EventListener);
        };
    }, []);

    // Fetch class group for class login
    useEffect(() => {
        const fetchClassGroup = async () => {
            const isClassLogin = currentUserInfo?.loginType === 'class';
            if (!isClassLogin) return;

            const classCode = currentUserInfo?.class_code || currentUserInfo?.classCode || currentUserInfo?.class_id;
            if (!classCode) return;

            setLoadingClassGroup(true);
            try {
                // Class group ID format: class_code + "01"
                const classGroupId = classCode + '01';
                console.log('[Dashboard] Fetching class group for class login:', classGroupId);

                // Get UserSig and login to TIM
                const userId = currentUserInfo?.user_id || classCode;
                if (!userId) {
                    setLoadingClassGroup(false);
                    return;
                }

                const userSig = await invoke<string>('get_user_sig', { userId });
                const loginSuccess = await loginTIM(userId, userSig);

                if (loginSuccess) {
                    // Get all TIM groups to find the class group
                    const timGroups = await getTIMGroups();
                    const foundGroup = timGroups.find((g: any) => g.groupID === classGroupId);

                    if (foundGroup) {
                        const groupInfo: GroupInfo = {
                            group_id: foundGroup.groupID,
                            group_name: foundGroup.name || `${currentUserInfo?.class_name || classCode}的班级群`,
                            face_url: foundGroup.avatar || '',
                            is_class_group: true,
                            classid: classCode
                        };
                        setClassGroup(groupInfo);
                        console.log('[Dashboard] Found class group:', groupInfo);
                    } else {
                        console.warn('[Dashboard] Class group not found in TIM:', classGroupId);
                    }
                } else {
                    console.warn('[Dashboard] TIM login failed for class login');
                }
            } catch (err) {
                console.error('[Dashboard] Failed to fetch class group for class login:', err);
            } finally {
                setLoadingClassGroup(false);
            }
        };

        fetchClassGroup();
    }, [currentUserInfo]);

    // 监听接收通知开关状态变化
    useEffect(() => {
        const handleNotificationSettingChange = (event: Event) => {
            const customEvent = event as CustomEvent;
            const enabled = customEvent.detail?.receiveNotification;
            if (enabled !== undefined) {
                setReceiveNotification(enabled);
            }
        };

        window.addEventListener('notification-setting-changed', handleNotificationSettingChange as EventListener);

        // 从服务器响应中获取状态（通过ClassInfoModal加载时会触发事件）
        // 这里不依赖localStorage，因为状态应该从服务器获取

        return () => {
            window.removeEventListener('notification-setting-changed', handleNotificationSettingChange as EventListener);
        };
    }, [currentUserInfo]);

    // 连接WebSocket并监听通知消息（仅班级登录）
    useEffect(() => {
        const isClassLogin = currentUserInfo?.loginType === 'class';
        if (!isClassLogin) return;

        const classCode = currentUserInfo?.class_code || currentUserInfo?.classCode || currentUserInfo?.class_id;
        if (!classCode) return;

        // 连接WebSocket
        connectWS(classCode);

        // 监听WebSocket消息
        const handleWSMessage = (event: Event) => {
            const customEvent = event as CustomEvent;
            const msgStr = customEvent.detail;
            if (!msgStr) return;

            try {
                const msg = JSON.parse(msgStr);
                // 检查是否是通知消息且是发给当前班级的
                if (msg.type === 'notification' && msg.class_id === classCode && receiveNotification) {
                    console.log('[Dashboard] Received notification, opening text message window');
                    setShowTextMessageWindow(true);
                } else if (msg.type === "monitor") {
                    console.log("[Dashboard] Received Monitor Command:", msg);
                    const action = msg.action;
                    const ts = Math.floor(Date.now() / 1000);
                    // Use the group_id from the received message (from teacher), fallback to classCode
                    const targetGroupId = msg.group_id || classCode;

                    if (action === "start_stream") {
                        console.log("[Dashboard] Starting Remote Monitoring...");
                        const streamName = `live/${targetGroupId}_${ts}`;
                        const publishUrl = `srt://47.100.126.194:10080?streamid=#!::r=${streamName},m=publish`;
                        const playUrl = `webrtc://47.100.126.194/live/${targetGroupId}_${ts}`;

                        invoke('start_stream', { pullUrl: publishUrl })
                            .then(() => console.log("[Dashboard] Streaming started successfully"))
                            .catch(err => console.error("[Dashboard] Failed to start stream:", err));

                        const response: any = {
                            type: "camera_stream",
                            action: "start_pull",
                            class_id: classCode,
                            group_id: targetGroupId,  // Use the same group_id from teacher's request
                            stream_name: streamName,
                            pull_url: playUrl,
                            sender_id: currentUserInfo?.user_id || "",
                            sender_name: currentUserInfo?.name || "Class Terminal",
                            ts: ts
                        };
                        const wsPayload = JSON.stringify(response);
                        const wsMessage = wsPayload.startsWith('to:') ? wsPayload : `to:${targetGroupId}:${wsPayload}`;
                        sendMessageWS(wsMessage);

                    } else if (action === "stop_stream") {
                        console.log("[Dashboard] Stopping Remote Monitoring...");
                        invoke('stop_stream')
                            .then(() => console.log("[Dashboard] Streaming stopped successfully"))
                            .catch(err => console.error("[Dashboard] Failed to stop stream:", err));

                        const response: any = {
                            type: "camera_stream",
                            action: "stop_pull",
                            class_id: classCode,
                            group_id: targetGroupId,  // Use the same group_id from teacher's request
                            sender_id: currentUserInfo?.user_id || "",
                            ts: ts
                        };
                        const wsPayload = JSON.stringify(response);
                        const wsMessage = wsPayload.startsWith('to:') ? wsPayload : `to:${targetGroupId}:${wsPayload}`;
                        sendMessageWS(wsMessage);
                    }
                }
            } catch (e) {
                // 忽略解析错误
            }
        };

        window.addEventListener('ws-message', handleWSMessage);
        return () => {
            window.removeEventListener('ws-message', handleWSMessage);
        };
    }, [currentUserInfo, receiveNotification]);


    const handleMinimize = () => getCurrentWindow().minimize();
    const handleClose = () => invoke('exit_app');
    const handleMaximize = async () => {
        const win = getCurrentWindow();
        const max = await win.isMaximized();
        if (max) {
            win.unmaximize();
            setIsMaximized(false);
        } else {
            win.maximize();
            setIsMaximized(true);
        }
    };

    const handleToolClick = async (toolId: string) => {
        if (toolId === 'schedule') {
            setActiveApp('schedule');
        } else if (toolId === 'create_class_group') {
            setShowCreateClassGroup(true);
        } else if (toolId === 'create_normal_group') {
            setShowCreateNormalGroup(true);
        } else if (toolId === 'search_add') {
            setShowSearchAdd(true);
        } else if (toolId === 'school_info') {
            setShowSchoolInfo(true);
        } else if (toolId === 'file_manager') {
            try {
                const { createNewBox } = await import('../utils/DesktopManager');
                const box = await createNewBox();
                await invoke('open_file_box_window', { boxId: box.id });
                // Optional: Minimize main window if desired, or keep as is
            } catch (e) {
                console.error(e);
                alert('创建文件盒子失败');
            }
        } else {
            console.log('Tool clicked:', toolId);
        }
    };

    const handleLogout = () => {
        if (confirm("确定要退出登录吗？")) {
            // Update login preferences to disable auto login
            const prefsStr = localStorage.getItem('login_prefs');
            if (prefsStr) {
                try {
                    const prefs = JSON.parse(prefsStr);
                    prefs.autoLogin = false;
                    localStorage.setItem('login_prefs', JSON.stringify(prefs));
                } catch (e) {
                    console.error("Failed to update login prefs", e);
                }
            }

            // Clear credentials
            localStorage.removeItem('token');

            // Reload to reset app state and trigger Login view
            window.location.reload();
        }
    };

    // Placeholder content generators
    const renderContent = () => {
        if (!activeApp) return null;

        // Windows 11 Internal Window
        // Animation: Fly upwards slightly + Fade
        return (
            <div className="absolute inset-4 bottom-20 bg-white/95 backdrop-blur-3xl rounded-lg shadow-[0_0_20px_rgba(0,0,0,0.1),0_0_0_1px_rgba(0,0,0,0.05)] flex flex-col overflow-hidden animate-in fade-in slide-in-from-bottom-4 duration-200">
                {/* Window Header */}
                <div className="h-10 flex items-center justify-between px-4 shrink-0 bg-transparent select-none draggable-region">
                    <div className="flex items-center gap-3">
                        <span className="text-sm font-semibold text-gray-700">
                            {activeApp === 'folder' && '资源云盘'}
                            {activeApp === 'schedule' && '课程表'}
                            {activeApp === 'user' && '班级信息'}
                        </span>
                    </div>
                    {/* Internal Window Controls (simulated) */}
                    <div className="flex items-center">
                        <button className="w-9 h-8 flex items-center justify-center hover:bg-black/5 rounded text-gray-500" onClick={() => setActiveApp(null)}>
                            <Minus size={14} />
                        </button>
                        <button className="w-9 h-8 flex items-center justify-center hover:bg-black/5 rounded text-gray-500">
                            <Square size={12} />
                        </button>
                        <button className="w-9 h-8 flex items-center justify-center hover:bg-red-600 hover:text-white rounded text-gray-500 transition-colors" onClick={() => setActiveApp(null)}>
                            <X size={16} />
                        </button>
                    </div>
                </div>

                {/* Window Body */}
                <div className="flex-1 p-6 overflow-auto bg-[#f9f9f9]/80">
                    {activeApp === 'folder' && (
                        <div className="h-full flex flex-col">
                            <h2 className="text-xl font-semibold text-gray-800 mb-6 px-2">桌面管理</h2>

                            <div className="grid grid-cols-4 gap-4">
                                {/* Class Group (for class login) - shown as first item */}
                                {currentUserInfo?.loginType === 'class' && (
                                    loadingClassGroup ? (
                                        <div className="flex flex-col items-center justify-center p-4 rounded-xl border border-transparent">
                                            <div className="w-14 h-14 rounded-2xl flex items-center justify-center mb-3 bg-gray-100 animate-pulse"></div>
                                            <span className="text-sm font-medium text-gray-400">加载中...</span>
                                        </div>
                                    ) : classGroup ? (
                                        <button
                                            key="class_group"
                                            onDoubleClick={() => invoke('open_class_window', { groupclassId: classGroup.group_id })}
                                            className="flex flex-col items-center justify-center p-4 rounded-xl hover:bg-white hover:shadow-sm transition-all duration-200 group active:scale-95 border border-transparent hover:border-gray-200"
                                        >
                                            <div className="w-14 h-14 rounded-2xl flex items-center justify-center text-3xl mb-3 shadow-sm bg-indigo-100 text-indigo-600 group-hover:scale-110 transition-transform duration-300 overflow-hidden">
                                                {classGroup.face_url ? (
                                                    <img
                                                        src={classGroup.face_url}
                                                        alt=""
                                                        className="w-full h-full object-cover"
                                                    />
                                                ) : (
                                                    <span>👥</span>
                                                )}
                                            </div>
                                            <span className="text-sm font-medium text-gray-700 group-hover:text-gray-900 text-center">
                                                {classGroup.group_name || '班级群'}
                                            </span>
                                        </button>
                                    ) : null
                                )}

                                {/* Regular Tools */}
                                {[
                                    { id: 'file_manager', name: '文件管理', icon: '📂', color: 'bg-blue-100 text-blue-600' },
                                    { id: 'new_folder', name: '创建文件夹', icon: '➕', color: 'bg-yellow-100 text-yellow-600' },
                                    { id: 'countdown', name: '倒计时', icon: '⏳', color: 'bg-red-100 text-red-600' },
                                    { id: 'time', name: '时间', icon: '🕒', color: 'bg-purple-100 text-purple-600' },
                                    { id: 'school_info', name: '配置学校信息', icon: '🏫', color: 'bg-emerald-100 text-emerald-600' },
                                    { id: 'wallpaper', name: '壁纸', icon: '🖼️', color: 'bg-pink-100 text-pink-600' },
                                    { id: 'schedule', name: '教师课程表', icon: '📅', color: 'bg-orange-100 text-orange-600' },
                                    { id: 'calendar', name: '校历', icon: '🗓️', color: 'bg-cyan-100 text-cyan-600' },
                                    { id: 'table', name: '表格', icon: '📊', color: 'bg-gray-100 text-gray-600' },
                                    { id: 'text', name: '文本', icon: '📝', color: 'bg-indigo-100 text-indigo-600' },
                                    { id: 'image', name: '图片', icon: '🏞️', color: 'bg-rose-100 text-rose-600' },
                                    // New Tools
                                    { id: 'create_class_group', name: '创建班级群', icon: '🎓', color: 'bg-blue-100 text-blue-600' },
                                    { id: 'create_normal_group', name: '创建讨论组', icon: '💬', color: 'bg-green-100 text-green-600' },
                                    { id: 'search_add', name: '查找添加', icon: '🔍', color: 'bg-purple-100 text-purple-600' },
                                ].map((tool) => (
                                    <button
                                        key={tool.id}
                                        onClick={() => handleToolClick(tool.id)}
                                        className="flex flex-col items-center justify-center p-4 rounded-xl hover:bg-white hover:shadow-sm transition-all duration-200 group active:scale-95 border border-transparent hover:border-gray-200"
                                    >
                                        <div className={`w-14 h-14 rounded-2xl flex items-center justify-center text-3xl mb-3 shadow-sm ${tool.color} group-hover:scale-110 transition-transform duration-300`}>
                                            {tool.icon}
                                        </div>
                                        <span className="text-sm font-medium text-gray-700 group-hover:text-gray-900">{tool.name}</span>
                                    </button>
                                ))}
                            </div>
                        </div>
                    )}
                    {activeApp === 'schedule' && (
                        <div className="h-full flex flex-col">
                            <TeacherSchedule />
                        </div>
                    )}
                    {activeApp === 'user' && (
                        <div className="h-full flex flex-col items-center justify-center space-y-8 animate-in zoom-in-95 duration-300">
                            {(() => {
                                const className = currentUserInfo?.class_name || currentUserInfo?.className || currentUserInfo?.class || currentUserInfo?.class_label || currentUserInfo?.classname;
                                const grade = currentUserInfo?.grade || currentUserInfo?.grade_level || currentUserInfo?.class_grade || currentUserInfo?.gradeLevel;
                                const schoolName = currentUserInfo?.school_name || currentUserInfo?.schoolName || currentUserInfo?.school || currentUserInfo?.schoolid;
                                const classCode = currentUserInfo?.class_code || currentUserInfo?.class_id || currentUserInfo?.class_number || currentUserInfo?.classCode || currentUserInfo?.classId || currentUserInfo?.classid;
                                const displayName = [grade, className].filter(Boolean).join('') || className || grade || '未知班级';
                                const avatarSrc = currentUserInfo?.face_url || currentUserInfo?.class_avatar || currentUserInfo?.avatar || "/default_group_avatar.png";

                                return (
                                    <>
                                        {/* Avatar Section */}
                                        <div className="relative group cursor-pointer" onClick={() => setShowUserInfo(true)}>
                                            <div className="w-32 h-32 rounded-full overflow-hidden border-4 border-white shadow-2xl transition-transform duration-300 group-hover:scale-105">
                                                <img
                                                    src={avatarSrc}
                                                    alt="Class Avatar"
                                                    className="w-full h-full object-cover"
                                                />
                                                <div className="absolute inset-0 bg-black/30 flex items-center justify-center opacity-0 group-hover:opacity-100 transition-opacity">
                                                    <span className="text-white text-xs font-bold">查看班级信息</span>
                                                </div>
                                            </div>
                                            <div className="absolute bottom-1 right-1 w-6 h-6 bg-green-500 border-4 border-white rounded-full"></div>
                                        </div>

                                        {/* Info Section */}
                                        <div className="text-center space-y-2">
                                            <h2 className="text-3xl font-bold text-gray-800 tracking-tight">{displayName}</h2>
                                            <div className="flex flex-col gap-1 text-gray-500 font-medium">
                                                <span className="bg-blue-50 text-blue-600 px-3 py-1 rounded-full text-sm inline-block mx-auto">
                                                    {schoolName || '未知学校'}
                                                </span>
                                                <span className="text-sm mt-1">
                                                    班级编号: {classCode || '---'}
                                                </span>
                                            </div>
                                        </div>
                                    </>
                                );
                            })()}

                            {/* Actions Section */}
                            <div className="pt-4">
                                <button
                                    onClick={handleLogout}
                                    className="flex items-center gap-2 px-8 py-3 bg-red-50 hover:bg-red-100 text-red-600 rounded-xl font-bold transition-all hover:shadow-md active:scale-95"
                                >
                                    <LogOut size={20} />
                                    <span>退出登录</span>
                                </button>
                            </div>
                        </div>
                    )}

                </div>
            </div >
        );
    };

    return (
        <div className={`h-screen w-screen overflow-hidden relative font-['Segoe_UI',system-ui] select-none text-gray-800 bg-[#f3f3f3] transition-all duration-300 ${!isMaximized ? 'rounded-2xl border border-gray-400/30 shadow-[0_0_15px_rgba(0,0,0,0.3)]' : ''}`}>
            {/* Windows 11 Mica-like Background */}
            <div className="absolute inset-0 z-0 bg-cover bg-center opacity-80" style={{ backgroundImage: 'url("https://4.bing.com/th?id=OHR.BlueHourParis_EN-US8633396684_1920x1080.jpg&rf=LaDigue_1920x1080.jpg")' }}></div>
            <div className="absolute inset-0 z-0 bg-white/40 backdrop-blur-[50px]"></div>

            {/* Custom Title Bar (Windows Style) */}
            <div data-tauri-drag-region className="absolute top-0 left-0 right-0 h-8 flex items-center justify-between z-[100]">
                {/* Title */}
                <div className="px-3 text-xs text-gray-600 flex items-center gap-2 pointer-events-none">
                    <span className="font-semibold">Teacher Assistant</span>
                </div>

                {/* Window Controls (Windows 11 Style) */}
                <div className="flex items-start h-full">
                    <button onClick={handleMinimize} className="w-11 h-full flex items-center justify-center hover:bg-black/5 hover:text-black text-gray-600 transition-colors active:bg-black/10">
                        <Minus size={16} strokeWidth={1.5} />
                    </button>
                    <button onClick={handleMaximize} className="w-11 h-full flex items-center justify-center hover:bg-black/5 hover:text-black text-gray-600 transition-colors active:bg-black/10">
                        {isMaximized ? <Copy size={14} className="rotate-180" strokeWidth={1.5} /> : <Square size={14} strokeWidth={1.5} />}
                    </button>
                    <button onClick={handleClose} className="w-11 h-full flex items-center justify-center hover:bg-[#c42b1c] hover:text-white text-gray-600 transition-colors active:bg-[#b02619]">
                        <X size={16} strokeWidth={1.5} />
                    </button>
                </div>
            </div>

            {/* Main Desktop Area */}
            <div className="absolute inset-0 z-10 pt-8 pb-14">
                {/* Welcome Message (if no app open) */}
                {activeApp === null && (
                    <div className="h-full flex flex-col items-center justify-center text-center space-y-4 animate-in fade-in duration-500">
                        <div className="w-20 h-20 rounded-full bg-blue-100 flex items-center justify-center mb-4 text-blue-600 font-bold text-2xl shadow-xl">
                            {currentUserInfo?.name?.[0] || 'T'}
                        </div>
                        <h1 className="text-3xl font-semibold text-gray-800 drop-shadow-sm">
                            {new Date().getHours() < 12 ? '早上好' : '下午好'}，{currentUserInfo?.name || '老师'}
                        </h1>
                        <p className="text-gray-600 text-base">
                            {new Date().toLocaleDateString(undefined, { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' })}
                        </p>
                    </div>
                )}

                {renderContent()}
            </div>

            {/* Taskbar */}
            <Taskbar activeItem={activeApp} onItemClick={setActiveApp} />

            {/* Modals */}
            <CreateClassGroupModal
                isOpen={showCreateClassGroup}
                onClose={() => setShowCreateClassGroup(false)}
                userInfo={currentUserInfo}
                onSuccess={() => window.dispatchEvent(new CustomEvent('refresh-class-list'))}
            />
            <CreateNormalGroupModal
                isOpen={showCreateNormalGroup}
                onClose={() => setShowCreateNormalGroup(false)}
                userInfo={currentUserInfo}
                onSuccess={() => window.dispatchEvent(new CustomEvent('refresh-class-list'))}
            />
            <SearchAddModal
                isOpen={showSearchAdd}
                onClose={() => setShowSearchAdd(false)}
                userInfo={currentUserInfo}
            />
            <UserInfoModal
                isOpen={showUserInfo}
                onClose={() => setShowUserInfo(false)}
                userInfo={currentUserInfo}
            />
            <SchoolInfoModal
                isOpen={showSchoolInfo}
                onClose={() => setShowSchoolInfo(false)}
                userInfo={userInfo}
            />

            {/* 班级端文本消息窗口 */}
            {currentUserInfo?.loginType === 'class' && classGroup && (
                <ClassTextMessageWindow
                    classCode={currentUserInfo?.class_code || currentUserInfo?.classCode || currentUserInfo?.class_id || ''}
                    groupId={classGroup.group_id}
                    isOpen={showTextMessageWindow}
                    onClose={() => setShowTextMessageWindow(false)}
                    receiveNotification={receiveNotification}
                />
            )}
        </div>
    );
};

export default Dashboard;
