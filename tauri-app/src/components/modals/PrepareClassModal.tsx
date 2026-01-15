import { useState, useEffect } from 'react';
import { X, Bookmark, Send } from 'lucide-react';
import { useDraggable } from '../../hooks/useDraggable';
import { sendMessageWS } from '../../utils/websocket';

interface Props {
    isOpen: boolean;
    onClose: () => void;
    subject: string;
    time: string;
    classId?: string;
    groupId?: string;
}

const getCacheKey = (classId: string, subject: string, time: string) =>
    `prepare_class_${classId}_${subject}_${time}`;

const PrepareClassModal = ({ isOpen, onClose, subject, time, classId, groupId }: Props) => {
    const { style, handleMouseDown } = useDraggable();
    const [content, setContent] = useState("");
    const [sending, setSending] = useState(false);

    useEffect(() => {
        if (isOpen && classId) {
            const cached = localStorage.getItem(getCacheKey(classId, subject, time));
            if (cached) {
                setContent(cached);
            } else {
                setContent("");
            }
        }
    }, [isOpen, classId, subject, time]);

    const handleSend = async () => {
        if (!content.trim()) {
            alert("请输入课前准备内容");
            return;
        }
        if (!classId) {
            alert("班级ID缺失，无法发送");
            return;
        }
        if (!groupId) {
            alert("群组ID缺失，无法发送");
            return;
        }

        setSending(true);

        try {
            const userInfoStr = localStorage.getItem('user_info');
            let senderName = "老师";
            let senderId = "";
            let schoolId = "";

            if (userInfoStr) {
                try {
                    const u = JSON.parse(userInfoStr);
                    senderName = u.name || u.strName || u.data?.name || "老师";
                    senderId = u.teacher_unique_id || u.data?.teacher_unique_id || "";
                    schoolId = u.school_id || u.data?.school_id || "";
                } catch (e) { }
            }

            // Format date as YYYY-MM-DD
            const today = new Date();
            const dateStr = today.toISOString().split('T')[0];

            // Format time as HH:MM (extract start time only)
            let timeStr = time;
            const timeMatch = time.match(/(\d{1,2}):(\d{2})/);
            if (timeMatch) {
                const hour = timeMatch[1].padStart(2, '0');
                const minute = timeMatch[2];
                timeStr = `${hour}:${minute}`;
            }

            // Build message object according to server protocol
            const messageObj: Record<string, string> = {
                type: "prepare_class",
                class_id: classId,
                subject: subject,
                content: content.trim(),
                date: dateStr,
                time: timeStr,
                sender_name: senderName,
            };

            // Optional fields
            if (senderId) messageObj.sender_id = senderId;
            if (schoolId) messageObj.school_id = schoolId;

            // Format: to:<group_id>:<JSON message>
            const wsMessage = `to:${groupId}:${JSON.stringify(messageObj)}`;

            console.log('[PrepareClassModal] Sending:', wsMessage);
            sendMessageWS(wsMessage);

            localStorage.setItem(getCacheKey(classId, subject, time), content.trim());
            onClose();
        } catch (e) {
            console.error("Failed to send prepare class content", e);
            alert("发送失败，请重试");
        } finally {
            setSending(false);
        }
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/30 backdrop-blur-sm animate-in fade-in duration-200">
            <div
                style={style}
                className="bg-white rounded-2xl shadow-2xl w-[480px] overflow-hidden border border-gray-100 flex flex-col"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="px-5 py-4 flex items-center justify-between cursor-move select-none border-b border-gray-100 bg-gradient-to-r from-gray-50 to-white"
                >
                    <div className="flex items-center gap-3">
                        <div className="w-9 h-9 bg-blue-100 rounded-xl flex items-center justify-center">
                            <Bookmark size={18} className="text-blue-600" />
                        </div>
                        <h3 className="font-bold text-gray-800 text-lg">课前准备</h3>
                    </div>
                    <button
                        onClick={onClose}
                        className="w-8 h-8 flex items-center justify-center hover:bg-gray-100 rounded-lg transition-colors text-gray-400 hover:text-gray-600"
                    >
                        <X size={18} />
                    </button>
                </div>

                {/* Subject Tag */}
                <div className="px-5 pt-4 pb-2">
                    <div className="flex items-center gap-2 text-sm">
                        <span className="px-3 py-1.5 bg-blue-500 text-white rounded-lg font-semibold text-xs">
                            {subject}
                        </span>
                        <span className="text-gray-400">|</span>
                        <span className="text-gray-500 font-medium">{time}</span>
                    </div>
                </div>

                {/* Content */}
                <div className="px-5 py-3">
                    <label className="block text-gray-600 text-sm font-medium mb-2">请输入课前准备内容</label>
                    <textarea
                        value={content}
                        onChange={e => setContent(e.target.value)}
                        placeholder="请输入课前准备内容..."
                        className="w-full bg-gray-50 text-gray-800 border border-gray-200 rounded-xl px-4 py-3 text-sm focus:ring-2 focus:ring-blue-100 focus:border-blue-300 transition-all outline-none resize-none h-44 placeholder:text-gray-400"
                    />
                </div>

                {/* Footer */}
                <div className="px-5 py-4 flex justify-end gap-3 bg-gray-50/50 border-t border-gray-100">
                    <button
                        onClick={onClose}
                        className="px-5 py-2.5 text-sm font-medium text-gray-600 bg-white border border-gray-200 hover:bg-gray-50 hover:border-gray-300 rounded-xl transition-all"
                    >
                        取消
                    </button>
                    <button
                        onClick={handleSend}
                        disabled={!content.trim() || sending}
                        className={`px-6 py-2.5 text-sm font-bold text-white rounded-xl flex items-center gap-2 transition-all shadow-lg ${!content.trim() || sending
                            ? 'bg-gray-300 cursor-not-allowed shadow-none'
                            : 'bg-blue-500 hover:bg-blue-600 shadow-blue-200'
                            }`}
                    >
                        {sending && <div className="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin"></div>}
                        <Send size={16} />
                        确定
                    </button>
                </div>
            </div>
        </div>
    );
};

export default PrepareClassModal;
