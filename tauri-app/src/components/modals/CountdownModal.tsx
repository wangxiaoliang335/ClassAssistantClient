import { useState, useEffect } from 'react';
import { X, Clock, Calendar as CalendarIcon, Save } from 'lucide-react';
import { useDraggable } from '../../hooks/useDraggable';

interface CountdownModalProps {
    isOpen: boolean;
    onClose: () => void;
    initialDate?: string; // yyyy-MM-dd
    initialTitle?: string;
}

const CountdownModal = ({ isOpen, onClose, initialDate, initialTitle }: CountdownModalProps) => {
    const [targetDate, setTargetDate] = useState(initialDate || new Date(new Date().getFullYear(), 5, 7).toISOString().split('T')[0]); // Default to June 7th (Gaokao)
    const [title, setTitle] = useState(initialTitle || "距高考还有");
    const [daysLeft, setDaysLeft] = useState(0);
    const { style, handleMouseDown } = useDraggable();

    useEffect(() => {
        const calculateDays = () => {
            const now = new Date();
            const target = new Date(targetDate);
            const diffTime = target.getTime() - now.getTime();
            const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24));
            setDaysLeft(diffDays);
        };
        calculateDays();
        // Recalculate if date changes (in a real app, maybe interval too, but days don't change fast)
    }, [targetDate]);

    if (!isOpen) return null;

    const handleSave = () => {
        // Here you would save to localStorage or backend
        console.log("Saving countdown config:", { title, targetDate });
        alert("倒计时设置已更新！"); // Simulation
        onClose();
    };

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-200">
            <div
                style={style}
                className="bg-white rounded-2xl shadow-2xl w-[450px] overflow-hidden border border-gray-100 flex flex-col"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="bg-gradient-to-r from-red-500 to-rose-600 p-4 flex items-center justify-between text-white cursor-move select-none"
                >
                    <div className="flex items-center gap-2 font-bold text-lg">
                        <Clock size={20} />
                        <span>倒计时设置</span>
                    </div>
                    <button onClick={onClose} className="p-1 hover:bg-white/20 rounded-full transition-colors">
                        <X size={20} />
                    </button>
                </div>

                {/* Content */}
                <div className="p-8 flex flex-col items-center">
                    {/* Preview Card */}
                    <div className="bg-rose-50 border border-rose-100 rounded-xl p-6 w-full flex flex-col items-center justify-center mb-6 shadow-sm">
                        <span className="text-gray-500 font-medium mb-2">{title}</span>
                        <div className="flex items-baseline gap-1">
                            <span className="text-6xl font-bold text-rose-600">{daysLeft > 0 ? daysLeft : 0}</span>
                            <span className="text-gray-500 font-medium">天</span>
                        </div>
                        {daysLeft <= 0 && <span className="text-xs text-rose-500 mt-2 font-bold">已到达或过期!</span>}
                    </div>

                    {/* Settings */}
                    <div className="w-full space-y-4">
                        <div className="space-y-1.5">
                            <label className="text-xs font-semibold text-gray-500 flex items-center gap-1"><CalendarIcon size={12} /> 目标日期</label>
                            <input
                                type="date"
                                value={targetDate}
                                onChange={(e) => setTargetDate(e.target.value)}
                                className="w-full text-sm font-medium text-gray-700 bg-gray-50 border border-gray-200 rounded-lg p-2.5 outline-none focus:ring-2 focus:ring-rose-500/20 focus:border-rose-500"
                            />
                        </div>
                        <div className="space-y-1.5">
                            <label className="text-xs font-semibold text-gray-500">提示文字</label>
                            <input
                                type="text"
                                value={title}
                                onChange={(e) => setTitle(e.target.value)}
                                className="w-full text-sm font-medium text-gray-700 bg-gray-50 border border-gray-200 rounded-lg p-2.5 outline-none focus:ring-2 focus:ring-rose-500/20 focus:border-rose-500"
                                placeholder="例如：距高考还有"
                            />
                        </div>
                    </div>
                </div>

                {/* Footer */}
                <div className="p-4 bg-gray-50 border-t border-gray-100 flex justify-end gap-3">
                    <button
                        onClick={onClose}
                        className="px-4 py-2 text-sm font-medium text-gray-600 hover:bg-gray-200 rounded-lg transition-colors"
                    >
                        取消
                    </button>
                    <button
                        onClick={handleSave}
                        className="px-6 py-2 text-sm font-bold text-white bg-rose-600 hover:bg-rose-700 rounded-lg shadow-sm shadow-rose-200 flex items-center gap-2 transition-all hover:-translate-y-0.5"
                    >
                        <Save size={16} /> 保存设置
                    </button>
                </div>
            </div>
        </div>
    );
};

export default CountdownModal;
