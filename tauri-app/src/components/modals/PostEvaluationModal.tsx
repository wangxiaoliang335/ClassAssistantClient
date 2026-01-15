import { useState } from 'react';
import { X, ClipboardCheck, Send } from 'lucide-react';
import { useDraggable } from '../../hooks/useDraggable';
import { sendMessageWS } from '../../utils/websocket';

interface Props {
    isOpen: boolean;
    onClose: () => void;
    subject: string;
    classId?: string;
    groupId?: string;
}

const PostEvaluationModal = ({ isOpen, onClose, subject, classId, groupId }: Props) => {
    const { style, handleMouseDown } = useDraggable();
    const [content, setContent] = useState("");
    const [sending, setSending] = useState(false);

    const handleSend = async () => {
        if (!content.trim()) {
            alert("请输入课后评价内容");
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

            if (userInfoStr) {
                try {
                    const u = JSON.parse(userInfoStr);
                    senderName = u.name || u.strName || u.data?.name || "老师";
                    senderId = u.teacher_unique_id || u.data?.teacher_unique_id || "";
                } catch (e) { }
            }

            const messageObj: Record<string, string> = {
                type: "post_class_evaluation",
                class_id: classId,
                subject: subject,
                content: content.trim(),
                date: new Date().toISOString().split('T')[0],
                sender_name: senderName,
            };

            if (senderId) messageObj.sender_id = senderId;

            const wsMessage = `to:${groupId}:${JSON.stringify(messageObj)}`;

            console.log('[PostEvaluationModal] Sending:', wsMessage);
            sendMessageWS(wsMessage);

            setContent("");
            onClose();
        } catch (e) {
            console.error("Failed to send post-class evaluation", e);
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
                        <div className="w-9 h-9 bg-green-100 rounded-xl flex items-center justify-center">
                            <ClipboardCheck size={18} className="text-green-600" />
                        </div>
                        <h3 className="font-bold text-gray-800 text-lg">课后评价</h3>
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
                        <span className="px-3 py-1.5 bg-green-500 text-white rounded-lg font-semibold text-xs">
                            {subject}
                        </span>
                    </div>
                </div>

                {/* Content */}
                <div className="px-5 py-3">
                    <label className="block text-gray-600 text-sm font-medium mb-2">请输入课后评价内容</label>
                    <textarea
                        value={content}
                        onChange={e => setContent(e.target.value)}
                        placeholder="请输入课后评价内容..."
                        className="w-full bg-gray-50 text-gray-800 border border-gray-200 rounded-xl px-4 py-3 text-sm focus:ring-2 focus:ring-green-100 focus:border-green-300 transition-all outline-none resize-none h-44 placeholder:text-gray-400"
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
                            : 'bg-green-500 hover:bg-green-600 shadow-green-200'
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

export default PostEvaluationModal;
