import { useState, useEffect, useRef } from "react";
import { X, Building, MapPin, GraduationCap, Users, Hash, Camera, Loader2, Bell, Calendar, Mic, ClipboardList, BookOpen } from "lucide-react";
import { invoke } from "@tauri-apps/api/core";

interface UserInfoModalProps {
    isOpen: boolean;
    onClose: () => void;
    userInfo: any;
}

const UserInfoModal = ({ isOpen, onClose, userInfo: initialUserInfo }: UserInfoModalProps) => {
    const [classInfo, setClassInfo] = useState(initialUserInfo);
    const [isUploadingAvatar, setIsUploadingAvatar] = useState(false);
    const [receiveNotification, setReceiveNotification] = useState(true);
    const [linkTodaySchedule, setLinkTodaySchedule] = useState(false);
    const [enableIntercom, setEnableIntercom] = useState(false);
    const [linkHomework, setLinkHomework] = useState(false);
    const [linkPrepareClass, setLinkPrepareClass] = useState(false);
    const fileInputRef = useRef<HTMLInputElement>(null);

    useEffect(() => {
        setClassInfo(initialUserInfo);
        if (isOpen) {
            console.log('[ClassInfoModal] classInfo:', initialUserInfo);
        }
    }, [initialUserInfo, isOpen]);

    if (!isOpen) return null;

    const className = classInfo?.class_name || classInfo?.className || classInfo?.class || classInfo?.class_label || classInfo?.classname;
    const grade = classInfo?.grade || classInfo?.grade_level || classInfo?.class_grade || classInfo?.gradeLevel;
    const classCode = classInfo?.class_code || classInfo?.class_id || classInfo?.class_number || classInfo?.classCode || classInfo?.classId || classInfo?.classid;
    const schoolName = classInfo?.school_name || classInfo?.schoolName || classInfo?.school || classInfo?.schoolid;
    const schoolStage = classInfo?.school_stage || classInfo?.schoolStage || classInfo?.stage;
    const address = classInfo?.address || classInfo?.school_address || classInfo?.location;
    const displayName = [grade, className].filter(Boolean).join('') || className || grade || '未知班级';
    const avatarSrc = classInfo?.face_url || classInfo?.class_avatar || classInfo?.avatar || "/default_group_avatar.png";

    useEffect(() => {
        const fetchClassInfo = async () => {
            if (!isOpen || !classCode) return;
            try {
                const token = localStorage.getItem('token') || '';
                const resStr = await invoke<string>('get_class_info', {
                    classCode,
                    token
                });
                console.log('[ClassInfoModal] get_class_info response:', resStr);

                const parsed = JSON.parse(resStr);
                const data = parsed?.data || parsed;
                const code = Number(data?.code ?? -1);
                const message = data?.message || '';

                if (code === 200) {
                    setClassInfo((prev: any) => {
                        const updated = {
                            ...prev,
                            class_code: data.class_code || classCode,
                            class_name: data.class_name || prev?.class_name,
                            school_stage: data.school_stage || prev?.school_stage,
                            grade: data.grade || prev?.grade,
                            schoolid: data.schoolid || prev?.schoolid,
                            school_name: data.school_name || prev?.school_name,
                            address: data.address || prev?.address,
                            face_url: data.face_url || prev?.face_url,
                            // Preserve loginType so class-mode UI doesn't disappear.
                            loginType: prev?.loginType || data?.loginType
                        };

                        try {
                            const cachedStr = localStorage.getItem('user_info');
                            const cached = cachedStr ? JSON.parse(cachedStr) : {};
                            localStorage.setItem('user_info', JSON.stringify({ ...cached, ...updated }));
                        } catch (e) {
                            console.warn('[ClassInfoModal] Failed to update local cache:', e);
                        }

                        window.dispatchEvent(new CustomEvent('class-info-updated', { detail: updated }));
                        return updated;
                    });
                } else {
                    console.warn('[ClassInfoModal] 获取班级信息失败:', code, message);
                }
            } catch (err) {
                console.error('[ClassInfoModal] Failed to fetch class info:', err);
            }
        };

        fetchClassInfo();
    }, [isOpen, classCode]);

    // Load notification and schedule link settings from localStorage
    useEffect(() => {
        if (isOpen && classCode) {
            const notificationKey = `class_notification_${classCode}`;
            const scheduleKey = `class_link_schedule_${classCode}`;
            const intercomKey = `class_intercom_${classCode}`;
            const homeworkKey = `class_link_homework_${classCode}`;
            const prepareKey = `class_link_prepare_${classCode}`;

            const savedNotification = localStorage.getItem(notificationKey);
            const savedSchedule = localStorage.getItem(scheduleKey);
            const savedIntercom = localStorage.getItem(intercomKey);
            const savedHomework = localStorage.getItem(homeworkKey);
            const savedPrepare = localStorage.getItem(prepareKey);
            
            if (savedNotification !== null) {
                setReceiveNotification(savedNotification === 'true');
            }
            if (savedSchedule !== null) {
                setLinkTodaySchedule(savedSchedule === 'true');
            }
            if (savedIntercom !== null) {
                setEnableIntercom(savedIntercom === 'true');
            }
            if (savedHomework !== null) {
                setLinkHomework(savedHomework === 'true');
            }
            if (savedPrepare !== null) {
                setLinkPrepareClass(savedPrepare === 'true');
            }
        }
    }, [isOpen, classCode]);

    const handleToggleNotification = () => {
        const newValue = !receiveNotification;
        setReceiveNotification(newValue);
        if (classCode) {
            localStorage.setItem(`class_notification_${classCode}`, String(newValue));
        }
    };

    const handleToggleScheduleLink = () => {
        const newValue = !linkTodaySchedule;
        setLinkTodaySchedule(newValue);
        if (classCode) {
            localStorage.setItem(`class_link_schedule_${classCode}`, String(newValue));
        }
    };

    const handleToggleIntercom = () => {
        const newValue = !enableIntercom;
        setEnableIntercom(newValue);
        if (classCode) {
            localStorage.setItem(`class_intercom_${classCode}`, String(newValue));
        }
    };

    const handleToggleHomework = () => {
        const newValue = !linkHomework;
        setLinkHomework(newValue);
        if (classCode) {
            localStorage.setItem(`class_link_homework_${classCode}`, String(newValue));
        }
    };

    const handleTogglePrepareClass = () => {
        const newValue = !linkPrepareClass;
        setLinkPrepareClass(newValue);
        if (classCode) {
            localStorage.setItem(`class_link_prepare_${classCode}`, String(newValue));
        }
    };

    const handleAvatarClick = () => {
        fileInputRef.current?.click();
    };

    const handleFileChange = async (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file) return;

        // 验证文件类型
        if (!file.type.startsWith('image/')) {
            alert('请选择图片文件');
            return;
        }

        // 验证文件大小（例如限制 5MB）
        if (file.size > 5 * 1024 * 1024) {
            alert('图片大小不能超过 5MB');
            return;
        }

        if (!classCode) {
            alert('班级编号为空，无法上传头像');
            return;
        }

        try {
            setIsUploadingAvatar(true);

            // 读取文件并转换为 Base64
            const reader = new FileReader();
            reader.onloadend = async () => {
                try {
                    const base64String = reader.result as string;
                    // 确保有 data:image 前缀
                    let imageBase64 = base64String;
                    if (!imageBase64.startsWith('data:image')) {
                        const mimeType = file.type || 'image/png';
                        const pureBase64 = base64String.split(',')[1] || base64String;
                        imageBase64 = `data:${mimeType};base64,${pureBase64}`;
                    }

                    const token = localStorage.getItem('token') || '';
                    const resStr = await invoke<string>('update_class_avatar', {
                        classCode,
                        avatar: imageBase64,
                        token
                    });

                    console.log('[ClassInfoModal] update_class_avatar response:', resStr);

                    const parsed = JSON.parse(resStr);
                    const data = parsed?.data || parsed;
                    const code = Number(data?.code ?? -1);
                    const message = data?.message || '';

                    if (code === 200) {
                        const faceUrl = data.face_url || imageBase64;
                        
                        // 更新本地状态
                        setClassInfo((prev: any) => {
                            const updated = {
                                ...prev,
                                face_url: faceUrl
                            };

                            // 更新缓存
                            try {
                                const cachedStr = localStorage.getItem('user_info');
                                const cached = cachedStr ? JSON.parse(cachedStr) : {};
                                localStorage.setItem('user_info', JSON.stringify({ ...cached, ...updated }));
                            } catch (e) {
                                console.warn('[ClassInfoModal] Failed to update cache:', e);
                            }

                            // 通知其他组件更新
                            window.dispatchEvent(new CustomEvent('class-info-updated', { detail: updated }));
                            return updated;
                        });

                        // 显示成功提示（简单版，可以后续改为 Toast）
                        alert('班级头像更新成功');
                    } else {
                        alert(`上传头像失败: ${message || '未知错误'}`);
                    }
                } catch (err) {
                    console.error('[ClassInfoModal] Failed to upload avatar:', err);
                    alert('上传头像失败，请重试');
                } finally {
                    setIsUploadingAvatar(false);
                    // 清空文件输入，允许重复选择同一文件
                    if (fileInputRef.current) {
                        fileInputRef.current.value = '';
                    }
                }
            };

            reader.onerror = () => {
                alert('读取文件失败');
                setIsUploadingAvatar(false);
            };

            reader.readAsDataURL(file);
        } catch (err) {
            console.error('[ClassInfoModal] File read error:', err);
            alert('读取文件失败');
            setIsUploadingAvatar(false);
        }
    };

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/40 backdrop-blur-sm p-4 animate-in fade-in duration-200">
            <div className="bg-white rounded-2xl shadow-2xl w-full max-w-sm overflow-hidden flex flex-col">
                {/* Header / Banner */}
                <div className="h-24 bg-gradient-to-r from-blue-500 to-indigo-600 relative">
                    <button
                        onClick={onClose}
                        className="absolute top-3 right-3 p-1.5 bg-black/10 hover:bg-black/20 text-white rounded-full transition-colors z-10"
                    >
                        <X size={18} />
                    </button>
                    <div className="absolute -bottom-10 left-1/2 -translate-x-1/2">
                        <div
                            className="w-20 h-20 rounded-full border-4 border-white bg-gray-200 shadow-md overflow-hidden relative group cursor-pointer"
                            onClick={handleAvatarClick}
                        >
                            <img
                                src={avatarSrc}
                                alt="Class Avatar"
                                className={`w-full h-full object-cover transition-opacity ${isUploadingAvatar ? 'opacity-50' : ''}`}
                            />
                            
                            {/* Loading Overlay */}
                            {isUploadingAvatar && (
                                <div className="absolute inset-0 flex items-center justify-center bg-black/20">
                                    <Loader2 className="animate-spin text-white" size={24} />
                                </div>
                            )}

                            {/* Hover Overlay with Edit Icon */}
                            {!isUploadingAvatar && (
                                <div className="absolute inset-0 bg-black/40 flex items-center justify-center opacity-0 group-hover:opacity-100 transition-opacity">
                                    <Camera size={20} className="text-white" />
                                </div>
                            )}

                            {/* Edit Icon Badge (Bottom Right) */}
                            {!isUploadingAvatar && (
                                <div className="absolute bottom-0 right-0 w-6 h-6 bg-blue-500 rounded-full flex items-center justify-center border-2 border-white shadow-md">
                                    <Camera size={12} className="text-white" />
                                </div>
                            )}

                            <input
                                type="file"
                                ref={fileInputRef}
                                className="hidden"
                                accept="image/png,image/jpeg,image/jpg,image/gif,image/bmp"
                                onChange={handleFileChange}
                            />
                        </div>
                    </div>
                </div>

                {/* Body */}
                <div className="pt-12 pb-6 px-6 flex flex-col items-center">

                    {/* Name Section */}
                    <div className="flex items-center gap-2 justify-center w-full">
                        <h2 className="text-xl font-bold text-gray-800">{displayName}</h2>
                    </div>
                </div>

                <p className="text-xs text-gray-500 mt-2">班级编号: {classCode || '---'}</p>

                <div className="w-full mt-6 space-y-4">
                    {/* Address */}
                    <div className="flex items-center gap-3 p-3 bg-gray-50 rounded-xl border border-gray-100">
                        <div className="w-8 h-8 rounded-full bg-amber-100 text-amber-600 flex items-center justify-center">
                            <MapPin size={16} />
                        </div>
                        <div className="flex-1">
                            <p className="text-xs text-gray-400">地址</p>
                            <p className="text-sm font-medium text-gray-700">{address || '无法获取'}</p>
                        </div>
                    </div>

                    {/* School */}
                    <div className="flex items-center gap-3 p-3 bg-gray-50 rounded-xl border border-gray-100">
                        <div className="w-8 h-8 rounded-full bg-blue-100 text-blue-600 flex items-center justify-center">
                            <Building size={16} />
                        </div>
                        <div className="flex-1">
                            <p className="text-xs text-gray-400">学校</p>
                            <p className="text-sm font-medium text-gray-700">{schoolName || '无法获取'}</p>
                        </div>
                    </div>

                    {/* School Stage */}
                    <div className="flex items-center gap-3 p-3 bg-gray-50 rounded-xl border border-gray-100">
                        <div className="w-8 h-8 rounded-full bg-green-100 text-green-600 flex items-center justify-center">
                            <GraduationCap size={16} />
                        </div>
                        <div className="flex-1">
                            <p className="text-xs text-gray-400">学段</p>
                            <p className="text-sm font-medium text-gray-700">{schoolStage || '---'}</p>
                        </div>
                    </div>

                    {/* Grade */}
                    <div className="flex items-center gap-3 p-3 bg-gray-50 rounded-xl border border-gray-100">
                        <div className="w-8 h-8 rounded-full bg-indigo-100 text-indigo-600 flex items-center justify-center">
                            <GraduationCap size={16} />
                        </div>
                        <div className="flex-1">
                            <p className="text-xs text-gray-400">年级</p>
                            <p className="text-sm font-medium text-gray-700">{grade || '---'}</p>
                        </div>
                    </div>

                    {/* Class Name */}
                    <div className="flex items-center gap-3 p-3 bg-gray-50 rounded-xl border border-gray-100">
                        <div className="w-8 h-8 rounded-full bg-purple-100 text-purple-600 flex items-center justify-center">
                            <Users size={16} />
                        </div>
                        <div className="flex-1">
                            <p className="text-xs text-gray-400">班级</p>
                            <p className="text-sm font-medium text-gray-700">{className || displayName}</p>
                        </div>
                    </div>

                    {/* Class Code */}
                    <div className="flex items-center gap-3 p-3 bg-gray-50 rounded-xl border border-gray-100">
                        <div className="w-8 h-8 rounded-full bg-rose-100 text-rose-600 flex items-center justify-center">
                            <Hash size={16} />
                        </div>
                        <div className="flex-1">
                            <p className="text-xs text-gray-400">班级编号</p>
                            <p className="text-sm font-medium text-gray-700">{classCode || '---'}</p>
                        </div>
                    </div>

                    {/* Receive Notification Toggle */}
                    <div className="flex items-center justify-between p-3 bg-gray-50 rounded-xl border border-gray-100">
                        <div className="flex items-center gap-3 flex-1">
                            <div className="w-8 h-8 rounded-full bg-blue-100 text-blue-600 flex items-center justify-center">
                                <Bell size={16} />
                            </div>
                            <div className="flex-1">
                                <p className="text-xs text-gray-400">接收通知</p>
                            </div>
                        </div>
                        <button
                            onClick={handleToggleNotification}
                            className={`w-10 h-5 rounded-full flex items-center px-0.5 transition-colors duration-300 ${receiveNotification ? 'bg-blue-500' : 'bg-gray-300'}`}
                        >
                            <div className={`w-4 h-4 bg-white rounded-full shadow-sm transform transition-transform duration-300 ${receiveNotification ? 'translate-x-5' : 'translate-x-0'}`} />
                        </button>
                    </div>

                    {/* Link Today Schedule Toggle */}
                    <div className="flex items-center justify-between p-3 bg-gray-50 rounded-xl border border-gray-100">
                        <div className="flex items-center gap-3 flex-1">
                            <div className="w-8 h-8 rounded-full bg-orange-100 text-orange-600 flex items-center justify-center">
                                <Calendar size={16} />
                            </div>
                            <div className="flex-1">
                                <p className="text-xs text-gray-400">关联今日课表</p>
                            </div>
                        </div>
                        <button
                            onClick={handleToggleScheduleLink}
                            className={`w-10 h-5 rounded-full flex items-center px-0.5 transition-colors duration-300 ${linkTodaySchedule ? 'bg-blue-500' : 'bg-gray-300'}`}
                        >
                            <div className={`w-4 h-4 bg-white rounded-full shadow-sm transform transition-transform duration-300 ${linkTodaySchedule ? 'translate-x-5' : 'translate-x-0'}`} />
                        </button>
                    </div>

                    {/* Enable Intercom Toggle */}
                    <div className="flex items-center justify-between p-3 bg-gray-50 rounded-xl border border-gray-100">
                        <div className="flex items-center gap-3 flex-1">
                            <div className="w-8 h-8 rounded-full bg-purple-100 text-purple-600 flex items-center justify-center">
                                <Mic size={16} />
                            </div>
                            <div className="flex-1">
                                <p className="text-xs text-gray-400">开启对讲</p>
                            </div>
                        </div>
                        <button
                            onClick={handleToggleIntercom}
                            className={`w-10 h-5 rounded-full flex items-center px-0.5 transition-colors duration-300 ${enableIntercom ? 'bg-blue-500' : 'bg-gray-300'}`}
                        >
                            <div className={`w-4 h-4 bg-white rounded-full shadow-sm transform transition-transform duration-300 ${enableIntercom ? 'translate-x-5' : 'translate-x-0'}`} />
                        </button>
                    </div>

                    {/* Link Homework Toggle */}
                    <div className="flex items-center justify-between p-3 bg-gray-50 rounded-xl border border-gray-100">
                        <div className="flex items-center gap-3 flex-1">
                            <div className="w-8 h-8 rounded-full bg-emerald-100 text-emerald-600 flex items-center justify-center">
                                <ClipboardList size={16} />
                            </div>
                            <div className="flex-1">
                                <p className="text-xs text-gray-400">关联家庭作业</p>
                            </div>
                        </div>
                        <button
                            onClick={handleToggleHomework}
                            className={`w-10 h-5 rounded-full flex items-center px-0.5 transition-colors duration-300 ${linkHomework ? 'bg-blue-500' : 'bg-gray-300'}`}
                        >
                            <div className={`w-4 h-4 bg-white rounded-full shadow-sm transform transition-transform duration-300 ${linkHomework ? 'translate-x-5' : 'translate-x-0'}`} />
                        </button>
                    </div>

                    {/* Link Prepare Class Toggle */}
                    <div className="flex items-center justify-between p-3 bg-gray-50 rounded-xl border border-gray-100">
                        <div className="flex items-center gap-3 flex-1">
                            <div className="w-8 h-8 rounded-full bg-rose-100 text-rose-600 flex items-center justify-center">
                                <BookOpen size={16} />
                            </div>
                            <div className="flex-1">
                                <p className="text-xs text-gray-400">关联课前准备</p>
                            </div>
                        </div>
                        <button
                            onClick={handleTogglePrepareClass}
                            className={`w-10 h-5 rounded-full flex items-center px-0.5 transition-colors duration-300 ${linkPrepareClass ? 'bg-blue-500' : 'bg-gray-300'}`}
                        >
                            <div className={`w-4 h-4 bg-white rounded-full shadow-sm transform transition-transform duration-300 ${linkPrepareClass ? 'translate-x-5' : 'translate-x-0'}`} />
                        </button>
                    </div>
                </div>

            </div>
        </div>
    );
};

export default UserInfoModal;
