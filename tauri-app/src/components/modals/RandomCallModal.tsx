import { useState, useEffect, useRef } from 'react';
import { X, Shuffle, RotateCcw, Filter, ChevronDown, Check, Users } from 'lucide-react';
import { useDraggable } from '../../hooks/useDraggable';

interface RandomCallModalProps {
    isOpen: boolean;
    onClose: () => void;
    classId?: string;
    students?: string[]; // Fallback simple list
}

interface AttributeTable {
    id: string;
    name: string;
    headers: string[]; // e.g. ["学号", "姓名", "性别", "小组"]
    rows: any[]; // e.g. [{ "学号": "1001", "姓名": "张三", "性别": "男"... }]
}

const RandomCallModal = ({ isOpen, onClose, classId, students: initialStudents = [] }: RandomCallModalProps) => {
    // Data State
    const [tables, setTables] = useState<AttributeTable[]>([]);
    const [loading, setLoading] = useState(false);

    // Filter State
    const [selectedTableId, setSelectedTableId] = useState<string>('');
    const [selectedAttribute, setSelectedAttribute] = useState<string>('');
    const [filterValue, setFilterValue] = useState<string>('ALL');

    // Derived State
    const [targetStudents, setTargetStudents] = useState<string[]>([]);
    const [availableAttributes, setAvailableAttributes] = useState<string[]>([]);
    const [availableValues, setAvailableValues] = useState<string[]>([]);

    // Animation State
    const [currentName, setCurrentName] = useState("---");
    const [isRunning, setIsRunning] = useState(false);
    const [history, setHistory] = useState<string[]>([]);

    // UI Refs
    const intervalRef = useRef<any>(null);
    const { style, handleMouseDown } = useDraggable();

    // Fetch Tables on Open
    useEffect(() => {
        if (isOpen && classId) {
            fetchAttributeTables();
        } else if (isOpen && initialStudents.length > 0) {
            // Fallback mode if no classId but students passed
            setTargetStudents(initialStudents);
        } else if (!isOpen) {
            // Reset state on close
            setIsRunning(false);
            if (intervalRef.current) clearInterval(intervalRef.current);
            setCurrentName("---");
        }
    }, [isOpen, classId]);

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
                                    const row: any = { '学号': s.student_id, '姓名': s.student_name };
                                    fields.forEach((f: string) => {
                                        const specificKey = `${f}_${ef.filename}`;
                                        row[f] = s.scores_json_full?.[specificKey] ?? s.scores_json_full?.[f] ?? s[f] ?? "";
                                    });
                                    return row;
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
                                const row: any = { '学号': s.student_id, '姓名': s.student_name };
                                fields.forEach((f: string) => {
                                    row[f] = s.scores_json_full?.[f] ?? s[f] ?? "";
                                });
                                return row;
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

            // 2. Fetch Group Scores (Group Attributes)
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

                                const rows: any[] = [];
                                groupScores.forEach((g: any) => {
                                    if (g.students) {
                                        g.students.forEach((s: any) => {
                                            const row: any = {
                                                '学号': s.student_id,
                                                '姓名': s.student_name,
                                                '小组': g.group_name
                                            };
                                            fields.forEach((f: string) => {
                                                const specificKey = `${f}_${filename}`;
                                                row[f] = s.scores_json_full?.[specificKey] ?? s.scores_json_full?.[f] ?? "";
                                            });
                                            rows.push(row);
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
                        } else {
                            let inferredFields: string[] = [];
                            if (groupScores?.[0]?.students?.[0]?.scores_json_full) {
                                inferredFields = Object.keys(groupScores[0].students[0].scores_json_full).filter(k => !k.includes("_"));
                            }
                            const rawFields = (headerInfo.fields?.map((f: any) => typeof f === 'string' ? f : f.field_name) || inferredFields);
                            const fields = rawFields.filter((f: string) => !['总分', '小组总分'].includes(f));

                            const rows: any[] = [];
                            groupScores.forEach((g: any) => {
                                if (g.students) {
                                    g.students.forEach((s: any) => {
                                        const row: any = {
                                            '学号': s.student_id,
                                            '姓名': s.student_name,
                                            '小组': g.group_name
                                        };
                                        fields.forEach((f: string) => {
                                            row[f] = s.scores_json_full?.[f] ?? "";
                                        });
                                        rows.push(row);
                                    });
                                }
                            });

                            newTables.push({
                                id: `group_${headerInfo.id || gIdx}`,
                                name: headerInfo.exam_name || "未命名小组表",
                                headers: ['小组', '学号', '姓名', ...fields],
                                rows: rows
                            });
                        }
                    });
                }
            } catch (e) {
                console.error("Failed to fetch group scores:", e);
            }

            setTables(newTables);

            if (newTables.length > 0) {
                if (!newTables.find(t => t.id === selectedTableId)) {
                    setSelectedTableId(newTables[0].id);
                }
            } else {
                if (initialStudents.length > 0 && newTables.length === 0) {
                    setTargetStudents(initialStudents);
                } else {
                    setTargetStudents([]);
                }
            }

        } catch (e) {
            console.error(e);
            if (initialStudents.length > 0) setTargetStudents(initialStudents);
        } finally {
            setLoading(false);
        }
    };

    // Update Available Attributes when Table Changes
    useEffect(() => {
        if (!selectedTableId) return;
        const table = tables.find(t => t.id === selectedTableId);
        if (table) {
            // Exclude standard non-attribute fields if necessary, but user might want to filter by ID?
            // Usually attributes are columns like "Gender", "Group", "Score"
            const attrs = table.headers.filter(h => !['学号', '姓名', '总分'].includes(h));
            setAvailableAttributes(attrs);
            setSelectedAttribute(attrs.length > 0 ? attrs[0] : '');
        }
    }, [selectedTableId, tables]);

    // Update Available Values and Students when Attribute/Table Changes
    useEffect(() => {
        if (!selectedTableId) {
            // No table selected, maybe fallback students?
            return;
        }

        const table = tables.find(t => t.id === selectedTableId);
        if (!table) return;

        // If no attribute selected, all students in table
        if (!selectedAttribute) {
            setTargetStudents(table.rows.map(r => r['姓名']));
            return;
        }

        // Get unique values for the attribute
        const values = Array.from(new Set(table.rows.map(r => r[selectedAttribute]))).filter(v => v !== undefined && v !== "") as string[];
        setAvailableValues(['ALL', ...values]);

        // Reset filter value to ALL when attribute changes
        setFilterValue('ALL');

    }, [selectedAttribute, selectedTableId, tables]);


    // Filter Students when Value Changes
    useEffect(() => {
        if (!selectedTableId) return;
        const table = tables.find(t => t.id === selectedTableId);
        if (!table) return;

        let filtered = table.rows;
        if (selectedAttribute && filterValue !== 'ALL') {
            filtered = filtered.filter(r => String(r[selectedAttribute]) === filterValue);
        }

        setTargetStudents(filtered.map(r => r['姓名']));
    }, [filterValue, selectedAttribute, selectedTableId, tables]);


    const toggleRollCall = () => {
        if (targetStudents.length === 0) {
            alert("当前列表没有学生，请调整筛选条件");
            return;
        }

        if (isRunning) {
            // Stop
            if (intervalRef.current) clearInterval(intervalRef.current);
            setIsRunning(false);
            // Add to history
            if (currentName !== "---") {
                setHistory(prev => [currentName, ...prev].slice(0, 5));
            }
        } else {
            // Start
            setIsRunning(true);
            intervalRef.current = setInterval(() => {
                const randomName = targetStudents[Math.floor(Math.random() * targetStudents.length)];
                setCurrentName(randomName);
            }, 50); // Fast cycle
        }
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/20 backdrop-blur-sm animate-in fade-in duration-200">
            <div
                style={style}
                className="bg-white rounded-2xl shadow-xl w-[500px] overflow-hidden border border-gray-100 flex flex-col"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="bg-gradient-to-r from-blue-50 to-indigo-50 p-4 flex items-center justify-between border-b border-gray-100 cursor-move select-none"
                >
                    <div className="flex items-center gap-2 font-bold text-lg text-blue-700">
                        <div className="p-1.5 bg-blue-100 rounded-lg">
                            <Shuffle size={18} className="text-blue-600" />
                        </div>
                        <span>随机点名</span>
                    </div>
                    <button onClick={onClose} className="p-1.5 hover:bg-white text-gray-400 hover:text-gray-700 rounded-full transition-all">
                        <X size={20} />
                    </button>
                </div>

                {/* Filter Section */}
                <div className="px-6 py-4 bg-white border-b border-gray-50 flex gap-3">
                    {/* Table Select */}
                    <div className="flex-1">
                        <label className="text-xs font-bold text-gray-400 mb-1 block uppercase tracking-wide">筛选表格</label>
                        <div className="relative">
                            <select
                                className="w-full appearance-none bg-gray-50 border border-gray-200 text-gray-700 text-sm rounded-lg py-2 px-3 pr-8 focus:outline-none focus:border-blue-500 focus:ring-2 focus:ring-blue-100 transition-all font-medium"
                                value={selectedTableId}
                                onChange={(e) => setSelectedTableId(e.target.value)}
                                disabled={loading || tables.length === 0}
                            >
                                {tables.length === 0 && <option>暂无属性表</option>}
                                {tables.map(t => <option key={t.id} value={t.id}>{t.name}</option>)}
                            </select>
                            <Filter className="absolute right-2.5 top-2.5 text-gray-400 pointer-events-none" size={14} />
                        </div>
                    </div>

                    {/* Attribute Select */}
                    <div className="w-1/4">
                        <label className="text-xs font-bold text-gray-400 mb-1 block uppercase tracking-wide">属性</label>
                        <div className="relative">
                            <select
                                className="w-full appearance-none bg-gray-50 border border-gray-200 text-gray-700 text-sm rounded-lg py-2 px-3 pr-8 focus:outline-none focus:border-blue-500 focus:ring-2 focus:ring-blue-100 transition-all font-medium"
                                value={selectedAttribute}
                                onChange={(e) => setSelectedAttribute(e.target.value)}
                                disabled={availableAttributes.length === 0}
                            >
                                {availableAttributes.length === 0 && <option>-</option>}
                                {availableAttributes.map(a => <option key={a} value={a}>{a}</option>)}
                            </select>
                            <ChevronDown className="absolute right-2.5 top-2.5 text-gray-400 pointer-events-none" size={14} />
                        </div>
                    </div>

                    {/* Value Select */}
                    <div className="w-1/4">
                        <label className="text-xs font-bold text-gray-400 mb-1 block uppercase tracking-wide">条件</label>
                        <div className="relative">
                            <select
                                className="w-full appearance-none bg-gray-50 border border-gray-200 text-gray-700 text-sm rounded-lg py-2 px-3 pr-8 focus:outline-none focus:border-blue-500 focus:ring-2 focus:ring-blue-100 transition-all font-medium"
                                value={filterValue}
                                onChange={(e) => setFilterValue(e.target.value)}
                                disabled={availableValues.length === 0}
                            >
                                <option value="ALL">全部</option>
                                {availableValues.filter(v => v !== 'ALL').map(v => <option key={v} value={v}>{v}</option>)}
                            </select>
                            <Check className="absolute right-2.5 top-2.5 text-gray-400 pointer-events-none" size={14} />
                        </div>
                    </div>
                </div>

                {/* Main Display */}
                <div className="p-8 flex flex-col items-center justify-center bg-gradient-to-b from-white to-gray-50">
                    <div className="relative mb-8 group cursor-pointer" onClick={toggleRollCall}>
                        {/* Outer Ring */}
                        <div className={`w-52 h-52 rounded-full border-4 flex items-center justify-center transition-all duration-300 ${isRunning ? 'border-blue-400 shadow-[0_0_30px_rgba(59,130,246,0.3)]' : 'border-gray-100 shadow-inner'}`}>
                            {/* Spinner Ring */}
                            {isRunning && (
                                <div className="absolute inset-0 rounded-full border-4 border-t-blue-500 border-r-transparent border-b-blue-500 border-l-transparent animate-spin-slow opacity-50"></div>
                            )}

                            {/* Central Text */}
                            <div className="w-40 h-40 rounded-full bg-white shadow-lg flex items-center justify-center relative z-10">
                                <span className={`text-4xl font-bold bg-clip-text text-transparent bg-gradient-to-br from-gray-800 to-gray-600 transition-all duration-100 ${isRunning ? 'scale-110 blur-[0.5px]' : 'scale-100'}`}>
                                    {currentName}
                                </span>
                            </div>
                        </div>

                        {/* Count Badge */}
                        <div className="absolute top-0 right-0 bg-blue-50 text-blue-600 text-xs font-bold px-2 py-1 rounded-full border border-blue-100 flex items-center gap-1 shadow-sm">
                            <Users size={10} />
                            {targetStudents.length}人
                        </div>
                    </div>

                    <button
                        onClick={toggleRollCall}
                        disabled={targetStudents.length === 0}
                        className={`w-full max-w-xs py-3.5 rounded-xl font-bold text-white shadow-lg transition-all active:scale-95 flex items-center justify-center gap-2 transform hover:-translate-y-0.5 ${isRunning
                            ? 'bg-gradient-to-r from-red-500 to-pink-600 shadow-red-200'
                            : 'bg-gradient-to-r from-blue-600 to-indigo-600 shadow-blue-200'}`}
                    >
                        {isRunning ? (
                            <>停止点名</>
                        ) : (
                            <>START</>
                        )}
                    </button>
                </div>

                {/* History */}
                {history.length > 0 && (
                    <div className="bg-gray-50 border-t border-gray-100 p-4">
                        <div className="flex items-center gap-2 text-xs font-bold text-gray-400 mb-3 uppercase tracking-wider">
                            <RotateCcw size={12} />
                            最近被点到的同学
                        </div>
                        <div className="flex gap-2 flex-wrap">
                            {history.map((name, i) => (
                                <span key={i} className="px-3 py-1 bg-white border border-gray-200 text-gray-600 text-xs font-medium rounded-full shadow-sm">
                                    {name}
                                </span>
                            ))}
                        </div>
                    </div>
                )}
            </div>
        </div>
    );
};

export default RandomCallModal;
