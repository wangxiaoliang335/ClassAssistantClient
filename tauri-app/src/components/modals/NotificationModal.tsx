import { useState, useEffect } from 'react';
import { X, Clock, Trash2, History, ChevronLeft } from 'lucide-react';
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

    useEffect(() => {
        if (isOpen && classId) {
            if (readOnly) {
                const unread = getLatestNotifications() || [];
                const mapped = unread.map((item: any, idx: number) => ({
                    id: String(item.id || item.notification_id || Date.now() + idx),
                    content: item.content || item.message || "",
                    senderName: item.sender_name || item.senderName || "系统",
                    timestamp: item.timestamp ? Number(item.timestamp) : Date.now()
                }));
                setNotices(mapped.slice(0, MAX_NOTICES));
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
                if (msg.type === 'notification' && (!classId || msg.class_id === classId)) {
                    const notice: Notice = {
                        id: String(msg.notification_id || Date.now()),
                        content: msg.content || "",
                        senderName: msg.sender_name || "系统",
                        timestamp: Date.now()
                    };
                    setNotices(prev => [notice, ...prev].slice(0, MAX_NOTICES));
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
        const updated = notices.filter(n => n.id !== id);
        setNotices(updated);
        saveNotices(classId, updated);
    };

    if (!isOpen) return null;

    const displayName = className || groupName || classId || "班级";

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-200">
            <div
                style={style}
                className="bg-white/95 backdrop-blur-3xl rounded-lg shadow-[0_0_20px_rgba(0,0,0,0.1),0_0_0_1px_rgba(0,0,0,0.05)] w-[420px] max-h-[600px] overflow-hidden flex flex-col text-gray-800"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="p-4 flex items-center justify-between cursor-move select-none border-b border-gray-100"
                >
                    {showHistory ? (
                        <button onClick={() => setShowHistory(false)} className="p-1 hover:bg-black/5 rounded-full transition-colors text-gray-500">
                            <ChevronLeft size={18} />
                        </button>
                    ) : (
                        <button onClick={onClose} className="p-1 hover:bg-black/5 rounded-full transition-colors text-gray-500">
                            <X size={18} />
                        </button>
                    )}
                    <h3 className="font-bold text-gray-800 text-base">
                        {readOnly ? '班级通知' : (showHistory ? '发送历史' : `文本消息 | ${displayName}`)}
                    </h3>
                    {!showHistory && !readOnly ? (
                        <button
                            onClick={() => setShowHistory(true)}
                            className="p-1 hover:bg-black/5 rounded-full transition-colors text-gray-500"
                            title="查看发送历史"
                        >
                            <History size={18} />
                        </button>
                    ) : (
                        <div className="w-6"></div>
                    )}
                </div>

                {showHistory || readOnly ? (
                    /* History View */
                    <div className="flex-1 overflow-y-auto p-4 space-y-3 max-h-[450px]">
                        {notices.length > 0 ? (
                            notices.map(notice => (
                                <div key={notice.id} className="bg-gray-50 border border-gray-100 rounded-lg p-3 group relative">
                                    <p className="text-gray-800 text-sm leading-relaxed pr-6">{notice.content}</p>
                                    <div className="text-[10px] text-gray-400 mt-2 flex items-center gap-1">
                                        <Clock size={10} />
                                        {new Date(notice.timestamp).toLocaleString()}
                                    </div>
                                    {!readOnly && (
                                        <button
                                            onClick={() => handleDelete(notice.id)}
                                            className="absolute top-3 right-3 p-1 text-gray-400 hover:text-red-500 opacity-0 group-hover:opacity-100 transition-all"
                                        >
                                            <Trash2 size={14} />
                                        </button>
                                    )}
                                </div>
                            ))
                        ) : (
                            <div className="text-center text-gray-400 py-10">
                                暂无通知
                            </div>
                        )}
                    </div>
                ) : (
                    /* Send View */
                    <>
                        <div className="p-4">
                            <textarea
                                value={newNotice}
                                onChange={e => setNewNotice(e.target.value)}
                                placeholder="请输入需要发送的文本消息"
                                className="w-full bg-gray-50 text-gray-800 border border-gray-200 rounded-lg px-4 py-3 text-sm focus:ring-2 focus:ring-blue-500/30 focus:border-blue-500 transition-all outline-none resize-none h-32 placeholder:text-gray-400"
                            />
                        </div>

                        <div className="p-4 flex justify-end gap-3">
                            <button
                                onClick={onClose}
                                className="px-5 py-2 text-sm font-medium text-gray-600 bg-gray-100 hover:bg-gray-200 rounded-lg transition-colors"
                            >
                                取消
                            </button>
                            <button
                                onClick={handleSend}
                                disabled={!newNotice.trim() || sending}
                                className={`px-6 py-2 text-sm font-bold text-white rounded-lg flex items-center gap-2 transition-all ${!newNotice.trim() || sending ? 'bg-gray-300 cursor-not-allowed' : 'bg-blue-600 hover:bg-blue-700'}`}
                            >
                                {sending && <div className="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin"></div>}
                                发送
                            </button>
                        </div>
                    </>
                )}
            </div>
        </div>
    );
};

export default NotificationModal;
