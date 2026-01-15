import { useEffect, useState, useRef } from 'react';
import { X, Maximize2, Minimize2 } from 'lucide-react';
import { useDraggable } from '../hooks/useDraggable';

interface Message {
    id: string;
    content: string;
    sender: string;
    senderName: string;
    timestamp: number;
}

interface ClassTextMessageWindowProps {
    classCode: string;
    groupId?: string; // 保留以兼容，但不再使用
    isOpen: boolean;
    onClose: () => void;
    receiveNotification: boolean; // 接收通知开关状态
}

const ClassTextMessageWindow = ({
    classCode,
    isOpen,
    onClose,
    receiveNotification
}: ClassTextMessageWindowProps) => {
    const { style, handleMouseDown } = useDraggable();
    const [mode, setMode] = useState<'minimal' | 'full'>('minimal'); // 1: 完整模式, 2: 极简模式（默认）
    const [messages, setMessages] = useState<Message[]>([]);
    const messagesEndRef = useRef<HTMLDivElement>(null);
    const messagesContainerRef = useRef<HTMLDivElement>(null);
    const [isDragging, setIsDragging] = useState(false);
    const dragStartY = useRef(0);
    const scrollStartY = useRef(0);

    // 监听WebSocket通知消息（只接收通知，不发送消息）
    useEffect(() => {
        if (!isOpen || !receiveNotification) return;

        const handleWSMessage = (event: Event) => {
            const customEvent = event as CustomEvent;
            const msgStr = customEvent.detail;
            if (!msgStr) return;

            try {
                const msg = JSON.parse(msgStr);
                // 检查是否是通知消息且是发给当前班级的
                if (msg.type === 'notification' && msg.class_id === classCode) {
                    console.log('[ClassTextMessageWindow] Received notification:', msg);
                    const notificationMessage: Message = {
                        id: `notification_${Date.now()}_${Math.random()}`,
                        content: msg.content || '',
                        sender: msg.sender_id || '',
                        senderName: msg.sender_name || '系统',
                        timestamp: Date.now()
                    };
                    // 保持最多100条消息
                    setMessages(prev => {
                        const newMessages = [...prev, notificationMessage];
                        return newMessages.slice(-100);
                    });

                    // 如果是极简模式，自动切换到完整模式以显示通知
                    if (mode === 'minimal') {
                        setMode('full');
                    }

                    // 自动滚动到底部
                    setTimeout(() => {
                        messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
                    }, 100);
                }
            } catch (e) {
                console.error('[ClassTextMessageWindow] Failed to parse WS message:', e);
            }
        };

        window.addEventListener('ws-message', handleWSMessage);
        return () => {
            window.removeEventListener('ws-message', handleWSMessage);
        };
    }, [isOpen, receiveNotification, classCode, mode]);

    // 切换模式
    const toggleMode = () => {
        setMode(prev => prev === 'minimal' ? 'full' : 'minimal');
    };

    // 完整模式下的拖动滚动
    const handleMouseDownOnMessages = (e: React.MouseEvent) => {
        if (mode !== 'full') return;
        setIsDragging(true);
        dragStartY.current = e.clientY;
        if (messagesContainerRef.current) {
            scrollStartY.current = messagesContainerRef.current.scrollTop;
        }
        e.preventDefault();
    };

    const handleMouseMove = (e: MouseEvent) => {
        if (!isDragging || mode !== 'full' || !messagesContainerRef.current) return;
        const deltaY = e.clientY - dragStartY.current;
        messagesContainerRef.current.scrollTop = scrollStartY.current - deltaY;
    };

    const handleMouseUp = () => {
        setIsDragging(false);
    };

    useEffect(() => {
        if (isDragging) {
            window.addEventListener('mousemove', handleMouseMove);
            window.addEventListener('mouseup', handleMouseUp);
            return () => {
                window.removeEventListener('mousemove', handleMouseMove);
                window.removeEventListener('mouseup', handleMouseUp);
            };
        }
    }, [isDragging]);

    // 自动滚动到底部
    useEffect(() => {
        if (mode === 'full') {
            setTimeout(() => {
                messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
            }, 100);
        }
    }, [messages, mode]);

    if (!isOpen) return null;

    const latestMessage = messages.length > 0 ? messages[messages.length - 1] : null;
    const displayMessages = mode === 'minimal' ? (latestMessage ? [latestMessage] : []) : messages;

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center pointer-events-none">
            <div
                className={`bg-white/95 backdrop-blur-3xl rounded-lg shadow-[0_0_20px_rgba(0,0,0,0.1),0_0_0_1px_rgba(0,0,0,0.05)] flex flex-col pointer-events-auto text-gray-800 ${
                    mode === 'minimal' ? 'w-96 h-24' : 'w-96 h-[600px]'
                } transition-all duration-300`}
                style={style}
            >
                {/* Header */}
                <div
                    className="flex items-center justify-between p-3 border-b border-gray-100 bg-transparent cursor-move"
                    onMouseDown={handleMouseDown}
                >
                    <div className="flex items-center gap-2">
                        <div className="w-8 h-8 rounded-full bg-blue-100 text-blue-600 flex items-center justify-center text-xs font-bold">
                            {classCode.slice(-2)}
                        </div>
                        <span className="text-gray-800 text-sm font-medium">班级通知</span>
                    </div>
                    <div className="flex items-center gap-1">
                        {/* 切换模式按钮 */}
                        <button
                            onClick={toggleMode}
                            className="p-1.5 hover:bg-black/5 rounded transition-colors"
                            title={mode === 'minimal' ? '切换到完整模式' : '切换到极简模式'}
                        >
                            {mode === 'minimal' ? (
                                <Maximize2 size={14} className="text-gray-500" />
                            ) : (
                                <Minimize2 size={14} className="text-gray-500" />
                            )}
                        </button>
                        {/* 关闭按钮 */}
                        <button
                            onClick={onClose}
                            className="p-1.5 hover:bg-black/5 rounded transition-colors"
                        >
                            <X size={14} className="text-gray-500" />
                        </button>
                    </div>
                </div>

                {/* Messages Area */}
                {mode === 'full' ? (
                    <div className="flex-1 flex flex-col overflow-hidden">
                        <div
                            ref={messagesContainerRef}
                            className="flex-1 overflow-y-auto p-4 space-y-3"
                            onMouseDown={handleMouseDownOnMessages}
                            style={{ cursor: isDragging ? 'grabbing' : 'grab' }}
                        >
                            {displayMessages.length === 0 ? (
                                <div className="text-gray-400 text-sm text-center py-8">
                                    暂无通知
                                </div>
                            ) : (
                                displayMessages.map((msg) => (
                                    <div key={msg.id} className="flex items-start gap-2">
                                        <div className="w-8 h-8 rounded-full bg-gray-100 flex items-center justify-center text-gray-600 text-xs flex-shrink-0 border border-gray-200">
                                            {msg.senderName[0] || '?'}
                                        </div>
                                        <div className="flex-1">
                                            <div className="bg-gray-50 border border-gray-100 rounded-lg px-3 py-2 text-gray-800 text-sm">
                                                {msg.content}
                                            </div>
                                            <div className="text-gray-500 text-xs mt-1">
                                                {msg.senderName} · {new Date(msg.timestamp).toLocaleTimeString()}
                                            </div>
                                        </div>
                                    </div>
                                ))
                            )}
                            <div ref={messagesEndRef} />
                        </div>
                    </div>
                ) : (
                    /* Minimal Mode - 只显示最新消息 */
                    <div className="flex items-center gap-2 p-3">
                        {latestMessage ? (
                            <>
                                <div className="w-8 h-8 rounded-full bg-gray-100 flex items-center justify-center text-gray-600 text-xs flex-shrink-0 border border-gray-200">
                                    {latestMessage.senderName[0] || '?'}
                                </div>
                                <div className="flex-1 min-w-0">
                                    <div className="bg-gray-50 border border-gray-100 rounded-lg px-3 py-2 text-gray-800 text-sm truncate">
                                        {latestMessage.content}
                                    </div>
                                </div>
                            </>
                        ) : (
                            <div className="text-gray-400 text-sm">暂无通知</div>
                        )}
                    </div>
                )}
            </div>
        </div>
    );
};

export default ClassTextMessageWindow;
