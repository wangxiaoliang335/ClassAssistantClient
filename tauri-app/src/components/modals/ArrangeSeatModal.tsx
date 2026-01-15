import { useState, useEffect } from 'react';
import { X, Users, ArrowUpDown, ChevronDown } from 'lucide-react';
import { useDraggable } from '../../hooks/useDraggable';

interface ArrangeSeatModalProps {
    isOpen: boolean;
    onClose: () => void;
    classId: string | undefined;
    onArrange: (arrangedStudents: StudentRow[]) => void;
}

interface AttributeTable {
    id: string;
    name: string;
    headers: string[];
    rows: StudentRow[];
}

interface StudentRow {
    id: string;
    name: string;
    [key: string]: any;
}

const ArrangeSeatModal: React.FC<ArrangeSeatModalProps> = ({
    isOpen,
    onClose,
    classId,
    onArrange
}) => {
    const { style, handleMouseDown } = useDraggable();

    // Data State
    const [tables, setTables] = useState<AttributeTable[]>([]);
    const [loading, setLoading] = useState(false);

    // Mode selection
    const [groupMode, setGroupMode] = useState<'group' | 'nogroup'>('nogroup');
    const [arrangeMethod, setArrangeMethod] = useState<string>('正序');

    // Table/Field selection
    const [selectedTableId, setSelectedTableId] = useState<string>('');
    const [selectedField, setSelectedField] = useState<string>('');
    const [availableFields, setAvailableFields] = useState<string[]>([]);

    const [isArranging, setIsArranging] = useState(false);

    // Update arrange method options when group mode changes
    useEffect(() => {
        if (groupMode === 'group') {
            setArrangeMethod('2人组排座');
        } else {
            setArrangeMethod('正序');
        }
    }, [groupMode]);

    // Fetch Tables on Open (same as RandomCallModal)
    useEffect(() => {
        if (isOpen && classId) {
            fetchAttributeTables();
        }
    }, [isOpen, classId]);

    // Update available fields when table selection changes
    useEffect(() => {
        const table = tables.find(t => t.id === selectedTableId);
        if (table) {
            // Filter out common ID fields
            const filtered = table.headers.filter(h =>
                !['学号', '姓名', '小组', '组号', 'id', 'name', 'student_id', 'student_name'].includes(h)
            );
            setAvailableFields(filtered);
            if (filtered.length > 0 && !filtered.includes(selectedField)) {
                setSelectedField(filtered[0]);
            }
        }
    }, [selectedTableId, tables]);

    // Fetch attribute tables (same logic as RandomCallModal)
    const fetchAttributeTables = async () => {
        setLoading(true);
        try {
            let targetClassId = classId || '';
            if (targetClassId.length === 11) targetClassId = targetClassId.substring(0, 9);

            const newTables: AttributeTable[] = [];

            // 1. Fetch Student Scores (Individual Attributes)
            try {
                const res = await fetch(`/api/student-scores?class_id=${targetClassId}`);
                const json = await res.json();
                if (json.code === 200 && json.data && json.data.headers) {
                    const list = Array.isArray(json.data.headers) ? json.data.headers : [json.data.headers];
                    list.forEach((item: any) => {
                        const filesList = (item.excel_file_url && Array.isArray(item.excel_file_url))
                            ? item.excel_file_url
                            : ((item.excel_files && Array.isArray(item.excel_files)) ? item.excel_files : []);

                        if (filesList.length > 0) {
                            filesList.forEach((ef: any, idx: number) => {
                                const rawFields = ef.fields || [];
                                const fields = rawFields
                                    .map((f: any) => typeof f === 'string' ? f : f.field_name)
                                    .filter((f: string) => f !== '总分');

                                const rows = (item.scores || []).map((s: any) => {
                                    const row: StudentRow = { id: s.student_id, name: s.student_name };
                                    fields.forEach((f: string) => {
                                        const specificKey = `${f}_${ef.filename}`;
                                        row[f] = s.scores_json_full?.[specificKey] ?? s.scores_json_full?.[f] ?? s[f] ?? "";
                                    });
                                    return row;
                                    return row;
                                }).filter((r: StudentRow) => {
                                    const cleanName = (r.name || '').replace(/\s+/g, '');
                                    return !cleanName.includes('讲台');
                                });

                                newTables.push({
                                    id: `score_${item.id}_${idx}`,
                                    name: ef.filename || item.exam_name || "未命名成绩单",
                                    headers: ['学号', '姓名', ...fields],
                                    rows: rows
                                });
                            });
                        } else {
                            const rawFields = item.fields || [];
                            const fields = rawFields
                                .map((f: any) => typeof f === 'string' ? f : f.field_name)
                                .filter((f: string) => f !== '总分');

                            const rows = (item.scores || []).map((s: any) => {
                                const row: StudentRow = { id: s.student_id, name: s.student_name };
                                fields.forEach((f: string) => {
                                    row[f] = s.scores_json_full?.[f] ?? s[f] ?? "";
                                });
                                return row;
                                return row;
                            }).filter((r: StudentRow) => {
                                const cleanName = (r.name || '').replace(/\s+/g, '');
                                return !cleanName.includes('讲台');
                            });

                            newTables.push({
                                id: `score_${item.id}`,
                                name: item.exam_name || "未命名成绩单",
                                headers: ['学号', '姓名', ...fields],
                                rows: rows
                            });
                        }
                    });
                }
            } catch (e) {
                console.error("Failed to fetch student scores:", e);
            }

            // 2. Fetch Group Scores
            try {
                const now = new Date();
                const year = now.getFullYear();
                const month = now.getMonth() + 1;
                const term = (month >= 8 || month <= 1) ? `${month >= 8 ? year : year - 1}-${month >= 8 ? year + 1 : year}-1` : `${year - 1}-${year}-2`;

                const res = await fetch(`/api/group-scores?class_id=${targetClassId}&term=${term}`);
                const json = await res.json();

                if (json.code === 200 && json.data) {
                    const groupList = Array.isArray(json.data) ? json.data : (json.data.header ? [json.data] : []);

                    groupList.forEach((tableData: any, gIdx: number) => {
                        const headerInfo = tableData.header || {};
                        const groupScores = tableData.group_scores || [];
                        const fileUrls = headerInfo.excel_file_url;

                        if (fileUrls && typeof fileUrls === 'object' && Object.keys(fileUrls).length > 0) {
                            Object.keys(fileUrls).forEach((filename, fIdx) => {
                                const fileInfo = fileUrls[filename];
                                const rawFields = fileInfo.fields || [];
                                const fields = rawFields.filter((f: string) => !['总分', '小组总分'].includes(f));

                                const rows: StudentRow[] = [];
                                groupScores.forEach((g: any) => {
                                    if (g.students) {
                                        g.students.forEach((s: any) => {
                                            const row: StudentRow = {
                                                id: s.student_id,
                                                name: s.student_name,
                                                '小组': g.group_name
                                            };
                                            fields.forEach((f: string) => {
                                                const specificKey = `${f}_${filename}`;
                                                row[f] = s.scores_json_full?.[specificKey] ?? s.scores_json_full?.[f] ?? "";
                                            });
                                            const cleanName = (s.student_name || '').replace(/\s+/g, '');
                                            if (cleanName && !cleanName.includes('讲台')) {
                                                rows.push(row);
                                            }
                                        });
                                    }
                                });

                                newTables.push({
                                    id: `group_${headerInfo.id || gIdx}_${fIdx}`,
                                    name: filename || "未命名小组表",
                                    headers: ['小组', '学号', '姓名', ...fields],
                                    rows: rows
                                });
                            });
                        }
                    });
                }
            } catch (e) {
                console.error("Failed to fetch group scores:", e);
            }

            setTables(newTables);
            if (newTables.length > 0) {
                setSelectedTableId(newTables[0].id);
            }
            console.log('[ArrangeSeat] Loaded', newTables.length, 'tables');
        } catch (e) {
            console.error("Failed to fetch tables:", e);
        } finally {
            setLoading(false);
        }
    };

    // Extract value from student for a given field
    const extractStudentValue = (student: StudentRow, field: string): number => {
        const val = student[field];
        if (typeof val === 'number') return val;
        if (typeof val === 'string') {
            const parsed = parseFloat(val);
            return isNaN(parsed) ? 0 : parsed;
        }
        return 0;
    };

    // Group wheel assign algorithm (from Qt source)
    const groupWheelAssign = (sorted: StudentRow[], groupSize: number): StudentRow[] => {
        const result: StudentRow[] = [];
        if (sorted.length === 0 || groupSize <= 0) return result;

        const n = sorted.length;
        const bucketCnt = groupSize;
        const buckets: StudentRow[][] = Array.from({ length: bucketCnt }, () => []);

        const base = Math.floor(n / bucketCnt);
        const rem = n % bucketCnt;
        let idx = 0;
        for (let b = 0; b < bucketCnt; b++) {
            const take = base + (b < rem ? 1 : 0);
            for (let k = 0; k < take && idx < n; k++, idx++) {
                buckets[b].push(sorted[idx]);
            }
        }

        let any = true;
        while (any) {
            any = false;
            for (let b = 0; b < bucketCnt; b++) {
                if (buckets[b].length === 0) continue;
                any = true;
                const pick = Math.floor(Math.random() * buckets[b].length);
                result.push(buckets[b][pick]);
                buckets[b].splice(pick, 1);
            }
        }

        return result;
    };

    // Core arrangement logic
    const handleArrange = () => {
        const table = tables.find(t => t.id === selectedTableId);
        if (!table || table.rows.length === 0) {
            alert('没有学生数据');
            return;
        }

        setIsArranging(true);

        try {
            let sorted = [...table.rows];

            // Sort by field value (descending by default)
            sorted.sort((a, b) => {
                return extractStudentValue(b, selectedField) - extractStudentValue(a, selectedField);
            });

            let ordered: StudentRow[];

            if (groupMode === 'nogroup') {
                if (arrangeMethod.includes('随机')) {
                    ordered = [...sorted];
                    for (let i = ordered.length - 1; i > 0; i--) {
                        const j = Math.floor(Math.random() * (i + 1));
                        [ordered[i], ordered[j]] = [ordered[j], ordered[i]];
                    }
                } else if (arrangeMethod.includes('正序')) {
                    sorted.sort((a, b) => {
                        return extractStudentValue(a, selectedField) - extractStudentValue(b, selectedField);
                    });
                    ordered = sorted;
                } else if (arrangeMethod.includes('倒序')) {
                    ordered = sorted;
                } else {
                    ordered = sorted;
                }
            } else {
                if (arrangeMethod.includes('2人组')) {
                    ordered = groupWheelAssign(sorted, 2);
                } else if (arrangeMethod.includes('4人组')) {
                    ordered = groupWheelAssign(sorted, 4);
                } else if (arrangeMethod.includes('6人组')) {
                    ordered = groupWheelAssign(sorted, 6);
                } else {
                    ordered = sorted;
                }
            }

            console.log('[ArrangeSeat] Arranged', ordered.length, 'students with method:', arrangeMethod);

            onArrange(ordered);
            onClose();
        } catch (err) {
            console.error('Arrangement error:', err);
            alert('排座失败');
        } finally {
            setIsArranging(false);
        }
    };

    const selectedTable = tables.find(t => t.id === selectedTableId);
    const studentCount = selectedTable?.rows.length || 0;

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 bg-black/20 backdrop-blur-sm flex items-center justify-center z-50">
            <div
                style={style}
                className="bg-white rounded-2xl w-[440px] shadow-2xl border border-gray-100 overflow-hidden"
            >
                {/* Header - Draggable */}
                <div
                    onMouseDown={handleMouseDown}
                    className="flex items-center justify-between p-4 border-b border-gray-100 bg-gradient-to-r from-teal-50 to-cyan-50 cursor-move"
                >
                    <h2 className="text-gray-800 font-bold flex items-center gap-2">
                        <div className="p-1.5 rounded-lg bg-teal-500 text-white">
                            <ArrowUpDown size={16} />
                        </div>
                        排座
                    </h2>
                    <button
                        onClick={onClose}
                        className="p-1.5 rounded-lg text-gray-400 hover:text-gray-600 hover:bg-gray-100 transition-colors"
                    >
                        <X size={18} />
                    </button>
                </div>

                {/* Content */}
                <div className="p-5 space-y-4">
                    {loading ? (
                        <div className="flex items-center justify-center py-8">
                            <div className="w-6 h-6 border-2 border-teal-500/30 border-t-teal-500 rounded-full animate-spin"></div>
                            <span className="ml-3 text-gray-500 text-sm">加载数据中...</span>
                        </div>
                    ) : (
                        <>
                            {/* Mode Selection Row */}
                            <div className="flex gap-4">
                                {/* Left: Group/NoGroup */}
                                <div className="flex-1">
                                    <label className="text-xs text-gray-500 mb-1.5 block font-medium">分组模式</label>
                                    <div className="relative">
                                        <select
                                            value={groupMode}
                                            onChange={(e) => setGroupMode(e.target.value as 'group' | 'nogroup')}
                                            className="w-full bg-gray-50 border border-gray-200 text-gray-700 rounded-xl p-2.5 text-sm focus:border-teal-500 focus:ring-2 focus:ring-teal-500/20 outline-none appearance-none cursor-pointer transition-all"
                                        >
                                            <option value="nogroup">不分组</option>
                                            <option value="group">小组</option>
                                        </select>
                                        <ChevronDown size={14} className="absolute right-3 top-1/2 -translate-y-1/2 text-gray-400 pointer-events-none" />
                                    </div>
                                </div>

                                {/* Right: Arrange Method */}
                                <div className="flex-1">
                                    <label className="text-xs text-gray-500 mb-1.5 block font-medium">排座方式</label>
                                    <div className="relative">
                                        <select
                                            value={arrangeMethod}
                                            onChange={(e) => setArrangeMethod(e.target.value)}
                                            className="w-full bg-gray-50 border border-gray-200 text-gray-700 rounded-xl p-2.5 text-sm focus:border-teal-500 focus:ring-2 focus:ring-teal-500/20 outline-none appearance-none cursor-pointer transition-all"
                                        >
                                            {groupMode === 'nogroup' ? (
                                                <>
                                                    <option value="正序">正序</option>
                                                    <option value="倒序">倒序</option>
                                                    <option value="随机排座">随机排座</option>
                                                </>
                                            ) : (
                                                <>
                                                    <option value="2人组排座">2人组排座</option>
                                                    <option value="4人组排座">4人组排座</option>
                                                    <option value="6人组排座">6人组排座</option>
                                                </>
                                            )}
                                        </select>
                                        <ChevronDown size={14} className="absolute right-3 top-1/2 -translate-y-1/2 text-gray-400 pointer-events-none" />
                                    </div>
                                </div>
                            </div>

                            {/* Table/Field Selection Row */}
                            <div className="flex gap-4">
                                {/* Table Selector */}
                                <div className="flex-1">
                                    <label className="text-xs text-gray-500 mb-1.5 block font-medium">数据来源</label>
                                    <div className="relative">
                                        <select
                                            value={selectedTableId}
                                            onChange={(e) => setSelectedTableId(e.target.value)}
                                            className="w-full bg-gray-50 border border-gray-200 text-gray-700 rounded-xl p-2.5 text-sm focus:border-teal-500 focus:ring-2 focus:ring-teal-500/20 outline-none appearance-none cursor-pointer transition-all"
                                        >
                                            {tables.length === 0 ? (
                                                <option value="">无可用数据</option>
                                            ) : (
                                                tables.map(t => (
                                                    <option key={t.id} value={t.id}>{t.name}</option>
                                                ))
                                            )}
                                        </select>
                                        <ChevronDown size={14} className="absolute right-3 top-1/2 -translate-y-1/2 text-gray-400 pointer-events-none" />
                                    </div>
                                </div>

                                {/* Field Selector */}
                                <div className="flex-1">
                                    <label className="text-xs text-gray-500 mb-1.5 block font-medium">排序字段</label>
                                    <div className="relative">
                                        <select
                                            value={selectedField}
                                            onChange={(e) => setSelectedField(e.target.value)}
                                            className="w-full bg-gray-50 border border-gray-200 text-gray-700 rounded-xl p-2.5 text-sm focus:border-teal-500 focus:ring-2 focus:ring-teal-500/20 outline-none appearance-none cursor-pointer transition-all"
                                        >
                                            {availableFields.length === 0 ? (
                                                <option value="">无可用字段</option>
                                            ) : (
                                                availableFields.map(f => (
                                                    <option key={f} value={f}>{f}</option>
                                                ))
                                            )}
                                        </select>
                                        <ChevronDown size={14} className="absolute right-3 top-1/2 -translate-y-1/2 text-gray-400 pointer-events-none" />
                                    </div>
                                </div>
                            </div>

                            {/* Preview Info */}
                            <div className="bg-gradient-to-r from-teal-50 to-cyan-50 rounded-xl p-3 border border-teal-100">
                                <div className="flex items-center gap-2 text-sm text-gray-600">
                                    <Users size={14} className="text-teal-500" />
                                    <span>共 <strong className="text-gray-800">{studentCount}</strong> 名学生</span>
                                    {selectedField && (
                                        <>
                                            <span className="text-gray-300">|</span>
                                            <span>按 <strong className="text-teal-600">{selectedField}</strong> {arrangeMethod}</span>
                                        </>
                                    )}
                                </div>
                            </div>
                        </>
                    )}
                </div>

                {/* Footer */}
                <div className="flex justify-end gap-3 p-4 border-t border-gray-100 bg-gray-50/50">
                    <button
                        onClick={onClose}
                        className="px-4 py-2 text-sm text-gray-500 hover:text-gray-700 hover:bg-gray-100 rounded-lg transition-colors"
                    >
                        取消
                    </button>
                    <button
                        onClick={handleArrange}
                        disabled={isArranging || availableFields.length === 0 || loading}
                        className="px-5 py-2 text-sm bg-gradient-to-r from-teal-500 to-cyan-500 hover:from-teal-600 hover:to-cyan-600 text-white rounded-lg transition-all shadow-sm shadow-teal-200 disabled:opacity-50 disabled:cursor-not-allowed flex items-center gap-2"
                    >
                        {isArranging ? (
                            <>
                                <div className="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin"></div>
                                排座中...
                            </>
                        ) : (
                            <>
                                <ArrowUpDown size={14} />
                                确定
                            </>
                        )}
                    </button>
                </div>
            </div>
        </div>
    );
};

export default ArrangeSeatModal;
