import { useState, useEffect } from 'react';
import { X, Trash2, History, ChevronLeft, Bell, MessageCircle, Inbox, Calendar, Search, Info } from 'lucide-react';
import { useDraggable } from '../../hooks/useDraggable';
import { sendMessageWS, getLatestNotifications } from '../../utils/websocket';

interface Props {
    isOpen: boolean;
    onClose: () => void;
    classId?: string;
    groupId?: string;
    groupName?: string;
    className?: string;
    readOnly?: boolean;
}

interface Notice {
    id: string;
    content: string;
    senderName: string;
    timestamp: number;
}

const MAX_NOTICES = 100;

const getStorageKey = (classId: string) => `notification_history_${classId}`;

const loadNotices = (classId: string): Notice[] => {
    try {
        const data = localStorage.getItem(getStorageKey(classId));
        return data ? JSON.parse(data) : [];
    } catch {
        return [];
    }
};

const saveNotices = (classId: string, notices: Notice[]) => {
    const trimmed = notices.slice(0, MAX_NOTICES);
    localStorage.setItem(getStorageKey(classId), JSON.stringify(trimmed));
};

const NotificationModal = ({ isOpen, onClose, classId, groupId, groupName, className, readOnly = false }: Props) => {
    const { style, handleMouseDown } = useDraggable();
    const [showHistory, setShowHistory] = useState(false);
    const [notices, setNotices] = useState<Notice[]>([]);
    const [newNotice, setNewNotice] = useState("");
    const [sending, setSending] = useState(false);
    const [searchQuery, setSearchQuery] = useState("");

    useEffect(() => {
        if (isOpen && classId) {
            if (readOnly) {
                // 优先从本地历史加载，再合并未读通知
                const history = loadNotices(classId);
                const unread = getLatestNotifications() || [];
                const mappedUnread = unread.map((item: any, i: number) => ({
                    id: String(item.id || item.notification_id || Date.now() + i),
                    content: item.content || item.message || "",
                    senderName: item.sender_name || item.senderName || "系统",
                    timestamp: item.timestamp ? Number(item.timestamp) : Date.now()
                }));
                
                // 合并并去重
                setNotices(() => {
                    const combined = [...mappedUnread, ...history];
                    const seen = new Set();
                    return combined.filter(n => {
                        if (seen.has(n.id)) return false;
                        seen.add(n.id);
                        return true;
                    }).slice(0, MAX_NOTICES);
                });
                setShowHistory(false);
            } else {
                setNotices(loadNotices(classId));
                setShowHistory(false);
            }
        }
    }, [isOpen, classId, readOnly]);

    useEffect(() => {
        if (!isOpen || !readOnly) return;
        const handleWSMessage = (event: Event) => {
            const customEvent = event as CustomEvent;
            const msgStr = customEvent.detail;
            if (!msgStr) return;
            try {
                const msg = JSON.parse(msgStr);
                // 更加健壮的 ID 匹配
                const isMatch = !classId || 
                                String(msg.class_id) === String(classId) || 
                                String(msg.group_id) === String(classId) ||
                                (msg.class_id && String(classId).startsWith(String(msg.class_id)));

                if (msg.type === 'notification' && isMatch) {
                    const notice: Notice = {
                        id: String(msg.notification_id || Date.now()),
                        content: msg.content || "",
                        senderName: msg.sender_name || "系统",
                        timestamp: Date.now()
                    };
                    setNotices(prev => {
                        const updated = [notice, ...prev].slice(0, MAX_NOTICES);
                        // 实时收到通知也存入本地，防止刷新丢失
                        if (classId) saveNotices(classId, updated);
                        return updated;
                    });
                }
            } catch (e) {
                // ignore
            }
        };
        window.addEventListener('ws-message', handleWSMessage as EventListener);
        return () => {
            window.removeEventListener('ws-message', handleWSMessage as EventListener);
        };
    }, [isOpen, readOnly, classId]);

    const handleSend = async () => {
        if (!newNotice.trim()) return;
        if (!classId) {
            alert("班级ID缺失，无法发送通知");
            return;
        }

        setSending(true);

        try {
            const userInfoStr = localStorage.getItem('user_info');
            let senderName = "老师";
            let senderId = "";

            if (userInfoStr) {
                try {
                    const u = JSON.parse(userInfoStr);
                    senderName = u.name || u.strName || u.data?.name || "老师";
                    senderId = u.teacher_unique_id || u.data?.teacher_unique_id || "";
                } catch (e) { }
            }

            const notificationObj: Record<string, string> = {
                type: "notification",
                class_id: classId,
                content: newNotice.trim(),
                content_text: "notification",
                sender_name: senderName,
            };

            if (senderId) notificationObj.sender_id = senderId;
            if (groupId) {
                notificationObj.group_id = groupId;
                notificationObj.unique_group_id = groupId;
            }
            if (groupName) notificationObj.group_name = groupName;

            let wsMessage = groupId
                ? `to:${groupId}:${JSON.stringify(notificationObj)}`
                : JSON.stringify(notificationObj);

            console.log('[NotificationModal] Sending:', wsMessage);
            sendMessageWS(wsMessage);

            // Save to history
            const newNoticeObj: Notice = {
                id: Date.now().toString(),
                content: newNotice.trim(),
                senderName,
                timestamp: Date.now()
            };
            const updatedNotices = [newNoticeObj, ...notices];
            setNotices(updatedNotices);
            saveNotices(classId, updatedNotices);

            setNewNotice("");
            onClose();
        } catch (e) {
            console.error("Failed to send notice", e);
            alert("发送失败，请重试");
        } finally {
            setSending(false);
        }
    };

    const handleDelete = (id: string) => {
        if (!classId) return;
        if (!confirm('确定要删除这条通知吗？')) return;
        const updated = notices.filter(n => n.id !== id);
        setNotices(updated);
        saveNotices(classId, updated);
    };

    if (!isOpen) return null;

    const displayName = className || groupName || classId || "班级";

    const filteredNotices = notices.filter(n => 
        n.content.toLowerCase().includes(searchQuery.toLowerCase()) || 
        n.senderName.toLowerCase().includes(searchQuery.toLowerCase())
    );

    const formatTime = (ts: number) => {
        const date = new Date(ts);
        const now = new Date();
        if (date.toDateString() === now.toDateString()) {
            return `今天 ${date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}`;
        }
        return `${date.getMonth() + 1}月${date.getDate()}日 ${date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}`;
    };

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-300 pointer-events-auto">
            <div
                style={style}
                className="bg-white/95 backdrop-blur-3xl rounded-3xl shadow-[0_20px_50px_rgba(0,0,0,0.2),0_0_0_1px_rgba(255,255,255,0.5)] w-[480px] max-h-[700px] overflow-hidden flex flex-col text-gray-800 border border-white/40"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="px-6 py-5 flex items-center justify-between cursor-move select-none border-b border-gray-100 bg-gradient-to-b from-white/50 to-transparent"
                >
                    <div className="flex items-center gap-3">
                        {showHistory ? (
                            <button onClick={() => setShowHistory(false)} className="p-2 hover:bg-black/5 rounded-xl transition-all text-gray-500">
                                <ChevronLeft size={20} />
                            </button>
                        ) : (
                            <div className="w-10 h-10 rounded-2xl bg-indigo-500 flex items-center justify-center shadow-lg shadow-indigo-500/20">
                                <Bell className="text-white" size={20} />
                            </div>
                        )}
                        <div>
                            <h3 className="font-bold text-gray-900 text-base tracking-tight">
                                {readOnly ? '班级通知中心' : (showHistory ? '发送历史' : `发布通知`)}
                            </h3>
                            {!showHistory && !readOnly && (
                                <p className="text-[10px] text-gray-400 font-bold uppercase tracking-wider">{displayName}</p>
                            )}
                        </div>
                    </div>
                    
                    <div className="flex items-center gap-2">
                        {!showHistory && !readOnly && (
                            <button
                                onClick={() => setShowHistory(true)}
                                className="p-2.5 hover:bg-indigo-50 text-gray-400 hover:text-indigo-600 rounded-xl transition-all"
                                title="查看发送历史"
                            >
                                <History size={20} />
                            </button>
                        )}
                        <button onClick={onClose} className="p-2.5 hover:bg-red-50 text-gray-400 hover:text-red-500 rounded-xl transition-all">
                            <X size={20} />
                        </button>
                    </div>
                </div>

                {showHistory || readOnly ? (
                    /* Notifications List View */
                    <div className="flex-1 overflow-hidden flex flex-col">
                        {/* Search Bar */}
                        <div className="px-6 py-3">
                            <div className="relative group">
                                <Search className="absolute left-3 top-1/2 -translate-y-1/2 text-gray-400 group-focus-within:text-indigo-500 transition-colors" size={14} />
                                <input 
                                    type="text"
                                    value={searchQuery}
                                    onChange={(e) => setSearchQuery(e.target.value)}
                                    placeholder="搜索通知内容或发布者..."
                                    className="w-full bg-gray-100 border-none rounded-xl pl-9 pr-4 py-2 text-xs focus:ring-2 focus:ring-indigo-500/20 transition-all outline-none"
                                />
                            </div>
                        </div>

                        <div className="flex-1 overflow-y-auto px-6 py-2 space-y-4 pb-8 scrollbar-hide">
                            <style>{`
                                .scrollbar-hide::-webkit-scrollbar { display: none; }
                            `}</style>
                            {filteredNotices.length > 0 ? (
                                filteredNotices.map((notice, idx) => (
                                    <div key={notice.id} className="animate-in fade-in slide-in-from-bottom-4 duration-300" style={{ animationDelay: `${idx * 50}ms` }}>
                                        <div className="flex items-center gap-2 mb-2 opacity-60 px-1">
                                            <Calendar size={10} className="text-indigo-500" />
                                            <span className="text-[10px] font-bold text-gray-500 uppercase tracking-wider">{formatTime(notice.timestamp)}</span>
                                        </div>
                                        <div className="bg-white hover:bg-gray-50 border border-gray-100 rounded-2xl p-4 shadow-sm transition-all group relative">
                                            <div className="flex items-center gap-2 mb-2">
                                                <div className="w-6 h-6 rounded-lg bg-indigo-50 text-indigo-500 flex items-center justify-center">
                                                    <MessageCircle size={12} fill="currentColor" />
                                                </div>
                                                <span className="text-xs font-bold text-gray-700">{notice.senderName}</span>
                                            </div>
                                            <p className="text-gray-600 text-sm leading-relaxed break-words whitespace-pre-wrap">
                                                {notice.content}
                                            </p>
                                            
                                            {!readOnly && (
                                                <button
                                                    onClick={() => handleDelete(notice.id)}
                                                    className="absolute top-4 right-4 p-1.5 text-gray-300 hover:text-red-500 hover:bg-red-50 rounded-lg opacity-0 group-hover:opacity-100 transition-all"
                                                >
                                                    <Trash2 size={14} />
                                                </button>
                                            )}
                                        </div>
                                    </div>
                                ))
                            ) : (
                                <div className="h-64 flex flex-col items-center justify-center space-y-4 opacity-40">
                                    <div className="w-20 h-20 rounded-full bg-gray-100 flex items-center justify-center">
                                        <Inbox size={40} className="text-gray-300" />
                                    </div>
                                    <div className="text-center px-10">
                                        <p className="text-gray-500 font-bold">暂无通知</p>
                                        <p className="text-[11px] text-gray-400 mt-1 leading-relaxed">
                                            {searchQuery ? '没有找到符合条件的通知' : '老师发布的新动态会即时同步到这里'}
                                        </p>
                                    </div>
                                </div>
                            )}
                        </div>
                    </div>
                ) : (
                    /* Send View */
                    <div className="flex-1 flex flex-col p-6 space-y-6">
                        <div className="flex-1 flex flex-col space-y-2">
                            <label className="text-xs font-bold text-gray-400 uppercase tracking-wider px-1">通知内容</label>
                            <textarea
                                value={newNotice}
                                onChange={e => setNewNotice(e.target.value)}
                                placeholder="在这里输入需要广播给全班同学的消息..."
                                className="flex-1 bg-gray-50 text-gray-800 border border-gray-100 rounded-2xl px-5 py-4 text-sm focus:ring-4 focus:ring-indigo-500/10 focus:border-indigo-500 transition-all outline-none resize-none placeholder:text-gray-300 shadow-inner"
                            />
                        </div>

                        <div className="flex items-center justify-between pt-2">
                            <div className="flex items-center gap-2 text-gray-400 italic">
                                <Info size={12} />
                                <span className="text-[10px]">发送后，班级终端将即时弹出弹窗提醒</span>
                            </div>
                            <div className="flex gap-3">
                                <button
                                    onClick={onClose}
                                    className="px-6 py-2.5 text-sm font-bold text-gray-500 hover:bg-gray-100 rounded-xl transition-all"
                                >
                                    取消
                                </button>
                                <button
                                    onClick={handleSend}
                                    disabled={!newNotice.trim() || sending}
                                    className={`px-8 py-2.5 text-sm font-bold text-white rounded-xl flex items-center gap-2 shadow-lg transition-all ${
                                        !newNotice.trim() || sending 
                                        ? 'bg-gray-200 shadow-none cursor-not-allowed text-gray-400' 
                                        : 'bg-indigo-600 hover:bg-indigo-700 shadow-indigo-200'
                                    }`}
                                >
                                    {sending ? (
                                        <div className="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin"></div>
                                    ) : (
                                        <MessageCircle size={16} />
                                    )}
                                    立即发布
                                </button>
                            </div>
                        </div>
                    </div>
                )}
            </div>
        </div>
    );
};

export default NotificationModal;
