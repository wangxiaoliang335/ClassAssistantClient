import { useState, useEffect } from 'react';
import { X, PieChart, BarChart3, Settings2, Plus, Trash2, Palette, RefreshCw, Check, AlertCircle } from 'lucide-react';
import { invoke } from '@tauri-apps/api/core';

interface ScoreAnalysisModalProps {
    isOpen: boolean;
    onClose: () => void;
    classId: string;
    onApply: (colorMap: Record<string, string>, mode: 'segment' | 'gradient' | 'none') => void;
}

interface AttributeTable {
    id: string;
    name: string;
    attributes: string[];
    rows: any[];
}

interface Segment {
    id: string;
    min: number;
    max: number;
    color: string;
    label: string;
}

const DEFAULT_SEGMENTS: Segment[] = [
    { id: '1', min: 0, max: 59, color: '#ef4444', label: '不及格 (<60)' },      // Red
    { id: '2', min: 60, max: 79, color: '#eab308', label: '及格 (60-79)' },    // Yellow
    { id: '3', min: 80, max: 89, color: '#3b82f6', label: '良好 (80-89)' },    // Blue
    { id: '4', min: 90, max: 100, color: '#22c55e', label: '优秀 (90-100)' },  // Green
];

const PRESET_COLORS = [
    '#ef4444', '#f97316', '#f59e0b', '#eab308', '#84cc16', '#22c55e',
    '#10b981', '#14b8a6', '#06b6d4', '#0ea5e9', '#3b82f6', '#6366f1',
    '#8b5cf6', '#a855f7', '#d946ef', '#ec4899', '#f43f5e', '#64748b'
];

const ScoreAnalysisModal = ({ isOpen, onClose, classId, onApply }: ScoreAnalysisModalProps) => {
    const [mode, setMode] = useState<'segment' | 'gradient'>('segment');
    const [tables, setTables] = useState<AttributeTable[]>([]);
    const [selectedTableId, setSelectedTableId] = useState<string>('');
    const [selectedAttribute, setSelectedAttribute] = useState<string>('');
    const [segments, setSegments] = useState<Segment[]>(DEFAULT_SEGMENTS);
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState<string | null>(null);

    // Initial fetch
    useEffect(() => {
        if (isOpen && classId) {
            fetchTables();
        }
    }, [isOpen, classId]);

    const fetchTables = async () => {
        setLoading(true);
        setError(null);
        try {
            let targetClassId = classId || '';
            if (targetClassId.length === 11) targetClassId = targetClassId.substring(0, 9);
            console.log(`[ScoreAnalysis] Fetching tables via fetch API for class: ${targetClassId}`);

            const newTables: AttributeTable[] = [];

            // 1. Fetch Student Scores
            try {
                const res = await fetch(`/api/student-scores?class_id=${targetClassId}`);
                const json = await res.json();
                console.log("[ScoreAnalysis] Student scores response code:", json.code);

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
                                    const row: any = { 'id': s.student_id, 'name': s.student_name };
                                    fields.forEach((f: string) => {
                                        const specificKey = `${f}_${ef.filename}`;
                                        row[f] = s.scores_json_full?.[specificKey] ?? s.scores_json_full?.[f] ?? s[f] ?? "";
                                    });
                                    return row;
                                });

                                newTables.push({
                                    id: `score_${item.id}_${idx}`,
                                    name: ef.filename || item.exam_name || "未命名成绩单",
                                    attributes: fields,
                                    rows: rows
                                });
                            });
                        } else {
                            const rawFields = item.fields || [];
                            const fields = rawFields
                                .map((f: any) => typeof f === 'string' ? f : f.field_name)
                                .filter((f: string) => f !== '总分');

                            const rows = (item.scores || []).map((s: any) => {
                                const row: any = { 'id': s.student_id, 'name': s.student_name };
                                fields.forEach((f: string) => {
                                    row[f] = s.scores_json_full?.[f] ?? s[f] ?? "";
                                });
                                return row;
                            });

                            newTables.push({
                                id: `score_${item.id}`,
                                name: item.exam_name || "未命名成绩单",
                                attributes: fields,
                                rows: rows
                            });
                        }
                    });
                }
            } catch (e) {
                console.error("[ScoreAnalysis] Failed to fetch student scores:", e);
            }

            // 2. Fetch Group Scores
            try {
                const now = new Date();
                const year = now.getFullYear();
                const month = now.getMonth() + 1;
                const term = (month >= 8 || month <= 1) ? `${month >= 8 ? year : year - 1}-${month >= 8 ? year + 1 : year}-1` : `${year - 1}-${year}-2`;
                console.log(`[ScoreAnalysis] Fetching group scores. Term: ${term}`);

                const res = await fetch(`/api/group-scores?class_id=${targetClassId}&term=${term}`);
                const json = await res.json();
                console.log("[ScoreAnalysis] Group scores response code:", json.code);

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
                                                'id': s.student_id,
                                                'name': s.student_name,
                                                'group': g.group_name
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
                                    attributes: fields,
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
                                            'id': s.student_id,
                                            'name': s.student_name,
                                            'group': g.group_name
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
                                attributes: fields,
                                rows: rows
                            });
                        }
                    });
                }
            } catch (e) {
                console.error("[ScoreAnalysis] Failed to fetch group scores:", e);
            }

            setTables(newTables);
            if (newTables.length > 0) {
                // Auto-select first if not selected
                if (!selectedTableId || !newTables.find(t => t.id === selectedTableId)) {
                    const first = newTables[0];
                    setSelectedTableId(first.id);
                    if (first.attributes.length > 0) {
                        setSelectedAttribute(first.attributes[0]);
                    }
                }
            }

        } catch (err: any) {
            console.error(err);
            setError("加载失败: " + (err.message || "未知错误"));
        } finally {
            setLoading(false);
        }
    };

    const handleTableChange = (tableId: string) => {
        setSelectedTableId(tableId);
        const table = tables.find(t => t.id === tableId);
        if (table && table.attributes.length > 0) {
            setSelectedAttribute(table.attributes[0]);
        } else {
            setSelectedAttribute('');
        }
    };

    const handleAddSegment = () => {
        const newId = Date.now().toString();
        setSegments([...segments, { id: newId, min: 0, max: 100, color: '#64748b', label: '新分段' }]);
    };

    const handleUpdateSegment = (id: string, field: keyof Segment, value: any) => {
        setSegments(segments.map(s => s.id === id ? { ...s, [field]: value } : s));
    };

    const handleDeleteSegment = (id: string) => {
        setSegments(segments.filter(s => s.id !== id));
    };

    const handleApply = async () => {
        if (!selectedTableId || !selectedAttribute) {
            setError("请先选择数据表和属性");
            return;
        }

        const table = tables.find(t => t.id === selectedTableId);
        if (!table) {
            setError("未找到选中的数据表");
            return;
        }

        setLoading(true);
        try {
            const colorMap: Record<string, string> = {};
            let minScore = Number.MAX_VALUE;
            let maxScore = Number.MIN_VALUE;

            const studentValues: { id: string, name: string, val: number }[] = [];

            table.rows.forEach(row => {
                const valStr = row[selectedAttribute];
                if (valStr !== undefined && valStr !== "" && valStr !== null) {
                    const val = Number(valStr);
                    if (!isNaN(val)) {
                        studentValues.push({ id: row.id, name: row.name, val });
                        if (val < minScore) minScore = val;
                        if (val > maxScore) maxScore = val;
                    }
                }
            });

            if (studentValues.length === 0) {
                // No valid values found
                setError("该属性下没有有效的数值数据");
                setLoading(false);
                return;
            }

            if (mode === 'segment') {
                studentValues.forEach(({ id, name, val }) => {
                    for (const seg of segments) {
                        if (val >= seg.min && val <= seg.max) {
                            if (id) colorMap[id] = seg.color;
                            // Fallback to name map if needed, but ID is preferred
                            break;
                        }
                    }
                });
            } else {
                // Gradient
                const range = maxScore - minScore || 1;
                studentValues.forEach(({ id, name, val }) => {
                    const normalized = (val - minScore) / range;
                    const color = getGradientColor(normalized);
                    if (id) colorMap[id] = color;
                });
            }

            console.log(`[ScoreAnalysis] Applied ${mode}. Mapped ${Object.keys(colorMap).length} students.`);
            onApply(colorMap, mode);
            onClose();

        } catch (err: any) {
            console.error(err);
            setError("应用失败: " + (err.message || "未知错误"));
        } finally {
            setLoading(false);
        }
    };

    const getGradientColor = (t: number): string => {
        // Jet/Rainbow Colormap: Blue -> Cyan -> Green -> Yellow -> Red
        // 0.0  0.125 0.375 0.625 0.875 1.0
        // We can approximate with 4 segments:
        // 0.0 - 0.25: Blue (0,0,255) -> Cyan (0,255,255)
        // 0.25 - 0.5: Cyan (0,255,255) -> Green (0,255,0)
        // 0.5 - 0.75: Green (0,255,0) -> Yellow (255,255,0)
        // 0.75 - 1.0: Yellow (255,255,0) -> Red (255,0,0)

        const clamp = (v: number) => Math.max(0, Math.min(255, Math.round(v)));

        let r = 0, g = 0, b = 0;

        if (t < 0.25) {
            // Blue -> Cyan
            const p = t / 0.25;
            r = 0;
            g = clamp(p * 255);
            b = 255;
        } else if (t < 0.5) {
            // Cyan -> Green
            const p = (t - 0.25) / 0.25;
            r = 0;
            g = 255;
            b = clamp(255 - p * 255);
        } else if (t < 0.75) {
            // Green -> Yellow
            const p = (t - 0.5) / 0.25;
            r = clamp(p * 255);
            g = 255;
            b = 0;
        } else {
            // Yellow -> Red
            const p = (t - 0.75) / 0.25;
            r = 255;
            g = clamp(255 - p * 255);
            b = 0;
        }

        return `rgb(${r}, ${g}, ${b})`;
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/50 backdrop-blur-sm">
            <div className="w-[800px] h-[650px] bg-white dark:bg-[#1a1b1e] rounded-xl shadow-2xl flex flex-col overflow-hidden animate-in fade-in zoom-in duration-200">

                {/* Header */}
                <div className="flex items-center justify-between px-6 py-4 border-b border-gray-200 dark:border-gray-800">
                    <div className="flex items-center gap-2">
                        <div className="bg-indigo-100 dark:bg-indigo-900/30 p-2 rounded-lg">
                            <PieChart className="w-5 h-5 text-indigo-600 dark:text-indigo-400" />
                        </div>
                        <h2 className="text-xl font-semibold text-gray-800 dark:text-gray-100">
                            数据可视化分析
                        </h2>
                    </div>
                    <button onClick={onClose} className="p-2 hover:bg-gray-100 dark:hover:bg-gray-800 rounded-full transition-colors">
                        <X className="w-5 h-5 text-gray-500" />
                    </button>
                </div>

                {/* Content */}
                <div className="flex-1 flex flex-col p-6 gap-6 overflow-hidden">

                    {/* Controls Row */}
                    <div className="flex gap-4 p-4 bg-gray-50 dark:bg-[#151618] rounded-xl border border-gray-200 dark:border-gray-800">
                        {/* Table Selector */}
                        <div className="flex-1 space-y-1">
                            <label className="text-xs font-semibold text-gray-500 uppercase">数据来源表</label>
                            <select
                                value={selectedTableId}
                                onChange={(e) => handleTableChange(e.target.value)}
                                className="w-full h-10 px-3 rounded-lg border border-gray-300 dark:border-gray-700 bg-white dark:bg-[#25262b] text-sm focus:ring-2 focus:ring-indigo-500 outline-none"
                            >
                                <option value="" disabled>选择数据表</option>
                                {tables.map(t => (
                                    <option key={t.id} value={t.id}>{t.name}</option>
                                ))}
                            </select>
                        </div>

                        {/* Attribute Selector */}
                        <div className="flex-1 space-y-1">
                            <label className="text-xs font-semibold text-gray-500 uppercase">分析属性</label>
                            <select
                                value={selectedAttribute}
                                onChange={(e) => setSelectedAttribute(e.target.value)}
                                disabled={!selectedTableId}
                                className="w-full h-10 px-3 rounded-lg border border-gray-300 dark:border-gray-700 bg-white dark:bg-[#25262b] text-sm focus:ring-2 focus:ring-indigo-500 outline-none disabled:opacity-50"
                            >
                                <option value="" disabled>选择属性</option>
                                {tables.find(t => t.id === selectedTableId)?.attributes.map(attr => (
                                    <option key={attr} value={attr}>{attr}</option>
                                ))}
                            </select>
                        </div>

                        {/* Mode Switch */}
                        <div className="flex items-end">
                            <div className="flex bg-gray-200 dark:bg-gray-800 p-1 rounded-lg">
                                <button
                                    onClick={() => setMode('segment')}
                                    className={`px-3 py-1.5 text-sm font-medium rounded-md transition-all ${mode === 'segment' ? 'bg-white text-indigo-600 shadow-sm' : 'text-gray-500 hover:text-gray-900'}`}
                                >
                                    <BarChart3 className="w-4 h-4 inline-block mr-1" />分断
                                </button>
                                <button
                                    onClick={() => setMode('gradient')}
                                    className={`px-3 py-1.5 text-sm font-medium rounded-md transition-all ${mode === 'gradient' ? 'bg-white text-indigo-600 shadow-sm' : 'text-gray-500 hover:text-gray-900'}`}
                                >
                                    <Palette className="w-4 h-4 inline-block mr-1" />热力图
                                </button>
                            </div>
                        </div>
                    </div>

                    {/* Mode Content */}
                    <div className="flex-1 overflow-auto border border-gray-200 dark:border-gray-800 rounded-xl p-4 bg-white dark:bg-[#151618]">

                        {mode === 'segment' ? (
                            <div className="space-y-4">
                                <div className="flex items-center justify-between mb-2">
                                    <h3 className="text-sm font-medium text-gray-700 dark:text-gray-300">分段配置</h3>
                                    <button onClick={handleAddSegment} className="text-xs flex items-center gap-1 text-indigo-600 hover:text-indigo-700 font-medium px-2 py-1 rounded hover:bg-indigo-50">
                                        <Plus className="w-3 h-3" /> 添加分段
                                    </button>
                                </div>

                                <div className="grid grid-cols-12 gap-2 text-xs font-semibold text-gray-500 px-2 uppercase mb-1">
                                    <div className="col-span-3">标签名称</div>
                                    <div className="col-span-2 text-center">最小值</div>
                                    <div className="col-span-2 text-center">最大值</div>
                                    <div className="col-span-2 text-center">颜色</div>
                                    <div className="col-span-3 text-right">操作</div>
                                </div>

                                <div className="space-y-2">
                                    {segments.map(seg => (
                                        <div key={seg.id} className="grid grid-cols-12 gap-2 items-center bg-gray-50 dark:bg-[#1a1b1e] p-2 rounded-lg border border-gray-100 dark:border-gray-800 group">
                                            <div className="col-span-3">
                                                <input
                                                    value={seg.label}
                                                    onChange={(e) => handleUpdateSegment(seg.id, 'label', e.target.value)}
                                                    className="w-full bg-transparent border-b border-transparent focus:border-indigo-500 outline-none text-sm"
                                                    placeholder="标签"
                                                />
                                            </div>
                                            <div className="col-span-2">
                                                <input
                                                    type="number"
                                                    value={seg.min}
                                                    onChange={(e) => handleUpdateSegment(seg.id, 'min', Number(e.target.value))}
                                                    className="w-full text-center bg-white dark:bg-[#25262b] border border-gray-200 rounded px-1 py-1 text-sm"
                                                />
                                            </div>
                                            <div className="col-span-2">
                                                <input
                                                    type="number"
                                                    value={seg.max}
                                                    onChange={(e) => handleUpdateSegment(seg.id, 'max', Number(e.target.value))}
                                                    className="w-full text-center bg-white dark:bg-[#25262b] border border-gray-200 rounded px-1 py-1 text-sm"
                                                />
                                            </div>
                                            <div className="col-span-2 flex justify-center relative">
                                                <div
                                                    className="w-8 h-8 rounded border border-gray-300 cursor-pointer shadow-sm relative overflow-hidden"
                                                    style={{ backgroundColor: seg.color }}
                                                >
                                                    <input
                                                        type="color"
                                                        value={seg.color}
                                                        onChange={(e) => handleUpdateSegment(seg.id, 'color', e.target.value)}
                                                        className="opacity-0 absolute inset-0 w-full h-full cursor-pointer"
                                                    />
                                                </div>
                                            </div>
                                            <div className="col-span-3 flex justify-end">
                                                <button
                                                    onClick={() => handleDeleteSegment(seg.id)}
                                                    className="p-1.5 text-gray-400 hover:text-red-600 hover:bg-red-50 rounded transition-colors"
                                                >
                                                    <Trash2 className="w-4 h-4" />
                                                </button>
                                            </div>
                                        </div>
                                    ))}
                                </div>
                            </div>
                        ) : (
                            <div className="flex flex-col items-center justify-center h-full space-y-8">
                                <div className="text-center space-y-2">
                                    <h3 className="text-lg font-medium text-gray-900 dark:text-white">渐变热力图</h3>
                                    <p className="text-sm text-gray-500">根据数值高低自动生成颜色渐变</p>
                                </div>

                                <div className="w-full max-w-md space-y-2">
                                    <div className="h-12 w-full rounded-lg bg-gradient-to-r from-[rgb(0,0,255)] via-[rgb(0,255,0)] to-[rgb(255,0,0)] shadow-inner border border-gray-200" />
                                    <div className="flex justify-between text-xs text-gray-500 font-medium">
                                        <span>最低分 (Blue)</span>
                                        <span>中间值 (Green/Yellow)</span>
                                        <span>最高分 (Red)</span>
                                    </div>
                                </div>

                                <div className="p-4 bg-blue-50 dark:bg-blue-900/10 rounded-lg text-sm text-blue-700 dark:text-blue-300 max-w-md">
                                    💡 系统将自动计算所选属性的最小值和最大值，并按比例映射到此色谱上。
                                </div>
                            </div>
                        )}

                    </div>
                    {error && (
                        <div className="flex items-center gap-2 text-red-600 bg-red-50 dark:bg-red-900/20 p-3 rounded-lg text-sm">
                            <AlertCircle className="w-4 h-4" />
                            {error}
                        </div>
                    )}
                </div>

                {/* Footer */}
                <div className="p-6 pt-0 flex justify-end gap-3">
                    <button onClick={onClose} className="px-4 py-2 text-sm font-medium text-gray-700 hover:bg-gray-100 rounded-lg transition-colors">
                        取消
                    </button>
                    <button
                        onClick={handleApply}
                        disabled={loading || !selectedTableId || !selectedAttribute}
                        className="px-6 py-2 text-sm font-medium text-white bg-indigo-600 hover:bg-indigo-700 disabled:opacity-50 disabled:cursor-not-allowed rounded-lg shadow-sm transition-colors flex items-center gap-2"
                    >
                        {loading && <RefreshCw className="w-4 h-4 animate-spin" />}
                        应用分析
                    </button>
                </div>
            </div>
        </div>
    );
};

export default ScoreAnalysisModal;
