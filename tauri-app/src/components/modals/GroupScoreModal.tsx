import { useState } from 'react';
import { X, Users, Award, TrendingUp, Plus, Minus } from 'lucide-react';
import { useDraggable } from '../../hooks/useDraggable';

interface GroupScoreModalProps {
    isOpen: boolean;
    onClose: () => void;
}

interface Group {
    id: number;
    name: string;
    score: number;
    members: string[]; // ["Name1", "Name2"]
}

const GroupScoreModal = ({ isOpen, onClose }: GroupScoreModalProps) => {
    // Mock Data
    const [groups, setGroups] = useState<Group[]>([
        { id: 1, name: '第一组', score: 85, members: ['张三', '李四', '王五', '赵六', '钱七', '孙八'] },
        { id: 2, name: '第二组', score: 92, members: ['周九', '吴十', '郑十一', '王十二', '冯十三', '陈十四'] },
        { id: 3, name: '第三组', score: 78, members: ['褚十五', '卫十六', '蒋十七', '沈十八', '韩十九', '杨二十'] },
        { id: 4, name: '第四组', score: 88, members: ['朱二一', '秦二二', '尤二三', '许二四', '何二五', '吕二六'] },
        { id: 5, name: '第五组', score: 95, members: ['施二七', '张二八', '孔二九', '曹三十', '严三一', '华三二'] },
        { id: 6, name: '第六组', score: 82, members: ['金三三', '魏三四', '陶三五', '姜三六', '戚三七', '谢三八'] },
    ]);
    const { style, handleMouseDown } = useDraggable();

    const handleScoreChange = (id: number, delta: number) => {
        setGroups(groups.map(g => {
            if (g.id === id) {
                return { ...g, score: g.score + delta };
            }
            return g;
        }));
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-200">
            <div
                style={style}
                className="bg-white rounded-2xl shadow-2xl w-[900px] h-[600px] flex flex-col overflow-hidden border border-gray-100"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="bg-gradient-to-r from-violet-500 to-purple-600 p-4 flex items-center justify-between text-white flex-shrink-0 cursor-move select-none"
                >
                    <div className="flex items-center gap-2 font-bold text-lg">
                        <Award size={20} />
                        <span>小组评价</span>
                    </div>
                    <button onClick={onClose} className="p-1 hover:bg-white/20 rounded-full transition-colors">
                        <X size={20} />
                    </button>
                </div>

                {/* Content */}
                <div className="flex-1 overflow-y-auto p-6 bg-gray-50/50">

                    {/* Top Stats / Controls */}
                    <div className="mb-6 flex items-center justify-between">
                        <div className="flex items-center gap-2 text-gray-500 text-sm">
                            <Users size={16} /> 共 {groups.length} 个小组
                        </div>
                        <button className="text-xs bg-white text-gray-600 border border-gray-200 hover:bg-gray-50 px-3 py-1.5 rounded-lg shadow-sm font-medium transition-colors">
                            重置所有分数
                        </button>
                    </div>

                    <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-5">
                        {groups.map((group) => (
                            <div key={group.id} className="bg-white rounded-xl border border-gray-100 shadow-sm flex flex-col overflow-hidden group hover:shadow-md transition-shadow">
                                {/* Group Header */}
                                <div className="p-4 border-b border-gray-50 flex items-center justify-between bg-gradient-to-br from-gray-50 to-white">
                                    <div className="font-bold text-gray-800 flex items-center gap-2">
                                        <div className="w-8 h-8 rounded-lg bg-violet-100 text-violet-600 flex items-center justify-center font-bold text-sm">
                                            {group.id}
                                        </div>
                                        {group.name}
                                    </div>
                                    <div className="flex items-center gap-1 text-2xl font-bold text-violet-600">
                                        {group.score}
                                        <span className="text-xs font-normal text-gray-400 mt-2">分</span>
                                    </div>
                                </div>

                                {/* Members */}
                                <div className="p-4 flex-1">
                                    <div className="flex flex-wrap gap-1.5">
                                        {group.members.map((m, i) => (
                                            <span key={i} className="text-xs bg-gray-50 text-gray-600 px-2 py-1 rounded text-center">
                                                {m}
                                            </span>
                                        ))}
                                    </div>
                                </div>

                                {/* Actions */}
                                <div className="p-3 bg-gray-50 border-t border-gray-100 flex items-center justify-between gap-3">
                                    <button
                                        onClick={() => handleScoreChange(group.id, -1)}
                                        className="flex-1 py-1.5 rounded-lg border border-red-200 text-red-600 hover:bg-red-50 flex items-center justify-center gap-1 transition-colors"
                                    >
                                        <Minus size={14} /> 扣分
                                    </button>
                                    <button
                                        onClick={() => handleScoreChange(group.id, 1)}
                                        className="flex-1 py-1.5 rounded-lg bg-violet-600 text-white hover:bg-violet-700 shadow-sm shadow-violet-200 flex items-center justify-center gap-1 transition-colors"
                                    >
                                        <Plus size={14} /> 加分
                                    </button>
                                </div>
                            </div>
                        ))}
                    </div>
                </div>
            </div>
        </div>
    );
};

export default GroupScoreModal;
