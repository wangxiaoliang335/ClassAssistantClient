import { useRef, useState, useEffect } from 'react';
import { X, Calendar, FileSpreadsheet, Maximize2, Minimize2, Loader2, RefreshCw, Users } from 'lucide-react';
import { invoke } from '@tauri-apps/api/core';
import * as XLSX from 'xlsx';
import { useDraggable } from '../../hooks/useDraggable';

interface DutyRosterModalProps {
    isOpen: boolean;
    onClose: () => void;
    groupId?: string;
}

interface DutyRosterResponse {
    code: number;
    message: string;
    data: {
        rows: string[][];
        requirement_row_index: number;
    };
}

const WEEKDAYS = ['周日', '周一', '周二', '周三', '周四', '周五', '周六'];

const DutyRosterModal = ({ isOpen, onClose, groupId }: DutyRosterModalProps) => {
    const [loading, setLoading] = useState(false);
    const [saving, setSaving] = useState(false);
    const [rows, setRows] = useState<string[][]>([]);
    const [requirementRowIndex, setRequirementRowIndex] = useState<number>(-1);
    const [isMinimalist, setIsMinimalist] = useState(false);
    const [error, setError] = useState<string | null>(null);
    const { style, handleMouseDown } = useDraggable();

    useEffect(() => {
        if (isOpen && groupId) {
            fetchRoster();
        }
    }, [isOpen, groupId]);

    const fetchRoster = async () => {
        if (!groupId) return;
        setLoading(true);
        setError(null);
        try {
            const resStr = await invoke<string>('fetch_duty_roster', { groupId });
            const res: DutyRosterResponse = JSON.parse(resStr);
            if (res.data) {
                setRows(res.data.rows || []);
                setRequirementRowIndex(res.data.requirement_row_index);
            }
        } catch (e) {
            console.error(e);
            setError("加载值日表失败");
        } finally {
            setLoading(false);
        }
    };

    const fileInputRef = useRef<HTMLInputElement>(null);

    const handleImport = () => {
        if (fileInputRef.current) {
            fileInputRef.current.click();
        }
    };

    const handleFileChange = async (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file) return;

        try {
            setLoading(true);
            const data = await file.arrayBuffer();
            const workbook = XLSX.read(data);
            const worksheetName = workbook.SheetNames[0];
            const worksheet = workbook.Sheets[worksheetName];
            const jsonData = XLSX.utils.sheet_to_json(worksheet, { header: 1 }) as any[][];

            if (!jsonData || jsonData.length === 0) {
                alert("Excel 文件为空或无法解析");
                return;
            }

            const newRows: string[][] = jsonData.map((row: any) => {
                const denseRow = Array.isArray(row) ? [...row] : [];
                return denseRow.map(cell => (cell === null || cell === undefined) ? "" : String(cell));
            });

            const maxCols = newRows.reduce((acc, row) => Math.max(acc, row.length), 0);
            const normalizedRows = newRows.map(row => {
                const arr = [...row];
                while (arr.length < maxCols) arr.push("");
                return arr;
            });

            if (confirm(`成功解析 ${normalizedRows.length} 行数据。是否覆盖当前值日表？`)) {
                setRows(normalizedRows);
                setSaving(true);
                try {
                    await invoke('save_duty_roster', {
                        groupId,
                        rows: normalizedRows,
                        requirementRowIndex: requirementRowIndex
                    });
                    alert("导入成功！");
                } catch (err) {
                    alert("导入后保存失败: " + String(err));
                } finally {
                    setSaving(false);
                }
            }
        } catch (error) {
            alert("导入失败: " + String(error));
        } finally {
            setLoading(false);
            if (fileInputRef.current) fileInputRef.current.value = "";
        }
    };

    const handleSave = async () => {
        if (!groupId) return;
        setSaving(true);
        try {
            await invoke('save_duty_roster', {
                groupId,
                rows,
                requirementRowIndex: requirementRowIndex
            });
        } catch (e) {
            console.error(e);
            alert("保存失败: " + String(e));
        } finally {
            setSaving(false);
        }
    };

    const handleCellChange = (rowIndex: number, colIndex: number, value: string) => {
        const newRows = [...rows];
        newRows[rowIndex] = [...newRows[rowIndex]];
        newRows[rowIndex][colIndex] = value;
        setRows(newRows);
    };

    const isRequirementRow = (index: number) => index === requirementRowIndex;

    const getTodayColumnIndex = () => {
        const day = new Date().getDay();
        if (day >= 1 && day <= 5) return day;
        return -1;
    };

    const renderMinimalistView = () => {
        const todayCol = getTodayColumnIndex();

        if (todayCol === -1) {
            return (
                <div className="flex flex-col items-center justify-center h-full text-gray-400 gap-3">
                    <Calendar size={48} className="text-gray-200" />
                    <span className="font-medium">今天不是工作日</span>
                </div>
            );
        }

        const taskList: { task: string; people: string[] }[] = [];

        for (let i = 1; i < rows.length; i++) {
            if (isRequirementRow(i)) continue;
            const row = rows[i];
            if (!row || row.length <= todayCol) continue;

            const taskName = row[0]?.trim();
            const person = row[todayCol]?.trim();

            if (taskName && person) {
                const lastTask = taskList[taskList.length - 1];
                if (lastTask && lastTask.task === taskName) {
                    if (!lastTask.people.includes(person)) {
                        lastTask.people.push(person);
                    }
                } else {
                    taskList.push({ task: taskName, people: [person] });
                }
            }
        }

        const requirementRow = requirementRowIndex >= 0 ? rows[requirementRowIndex] : null;

        return (
            <div className="p-6 h-full overflow-y-auto">
                <div className="flex items-center gap-3 mb-6">
                    <div className="w-10 h-10 bg-blue-100 rounded-xl flex items-center justify-center">
                        <Users size={20} className="text-blue-600" />
                    </div>
                    <div>
                        <h2 className="text-xl font-bold text-gray-800">今日值日</h2>
                        <p className="text-sm text-gray-400">{WEEKDAYS[new Date().getDay()]}</p>
                    </div>
                </div>

                <div className="space-y-3">
                    {taskList.length === 0 ? (
                        <div className="text-center py-12 text-gray-400">
                            <Calendar size={40} className="mx-auto mb-3 text-gray-200" />
                            今日无值日安排
                        </div>
                    ) : (
                        taskList.map((item, idx) => (
                            <div key={idx} className="flex bg-gray-50 border border-gray-100 rounded-xl overflow-hidden hover:border-gray-200 transition-colors">
                                <div className="w-28 p-4 bg-gradient-to-br from-gray-100 to-gray-50 font-bold text-gray-700 flex items-center justify-center border-r border-gray-100">
                                    {item.task}
                                </div>
                                <div className="flex-1 p-4 flex items-center flex-wrap gap-2">
                                    {item.people.map((p, pi) => (
                                        <span key={pi} className="bg-blue-100 text-blue-700 px-3 py-1.5 rounded-lg text-sm font-semibold">
                                            {p}
                                        </span>
                                    ))}
                                </div>
                            </div>
                        ))
                    )}
                </div>

                {requirementRow && (
                    <div className="mt-6 p-4 bg-amber-50 rounded-xl border border-amber-100">
                        <h3 className="font-bold mb-2 text-amber-700 flex items-center gap-2">
                            <span className="w-1.5 h-4 bg-amber-400 rounded-full"></span>
                            要求 & 备注
                        </h3>
                        <div className="text-sm text-amber-900/70 whitespace-pre-wrap">
                            {(() => {
                                const content = [];
                                if (requirementRow[0] && requirementRow[0].includes(',')) {
                                    content.push(...requirementRow[0].split(',').slice(1).map(s => s.trim()).filter(s => s));
                                }
                                for (let c = 1; c < requirementRow.length; c++) {
                                    if (requirementRow[c]?.trim()) content.push(requirementRow[c].trim());
                                }
                                return content.join(' ');
                            })()}
                        </div>
                    </div>
                )}
            </div>
        );
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/30 backdrop-blur-sm animate-in fade-in duration-200">
            <div
                style={style}
                className="bg-white rounded-2xl shadow-2xl w-[950px] h-[650px] flex flex-col overflow-hidden border border-gray-100"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="h-14 flex items-center justify-between px-5 bg-gradient-to-r from-gray-50 to-white border-b border-gray-100 cursor-move select-none"
                >
                    <div className="flex items-center gap-3">
                        <div className="w-9 h-9 bg-blue-100 rounded-xl flex items-center justify-center">
                            <Calendar size={18} className="text-blue-600" />
                        </div>
                        <h3 className="font-bold text-gray-800 text-lg">值日表</h3>
                    </div>
                    <div className="flex items-center gap-2">
                        <input
                            type="file"
                            ref={fileInputRef}
                            onChange={handleFileChange}
                            accept=".xlsx, .xls, .csv"
                            className="hidden"
                        />
                        <button
                            className="px-4 py-2 bg-blue-500 hover:bg-blue-600 text-white rounded-xl text-sm font-semibold flex items-center gap-2 transition-colors shadow-sm"
                            onClick={handleImport}
                        >
                            <FileSpreadsheet size={16} />
                            导入
                        </button>
                        <button
                            className={`px-4 py-2 rounded-xl text-sm font-semibold flex items-center gap-2 transition-colors ${isMinimalist
                                    ? 'bg-gray-100 text-gray-600 hover:bg-gray-200'
                                    : 'bg-gray-100 text-gray-600 hover:bg-gray-200'
                                }`}
                            onClick={() => setIsMinimalist(!isMinimalist)}
                        >
                            {isMinimalist ? <Maximize2 size={16} /> : <Minimize2 size={16} />}
                            {isMinimalist ? "完整视图" : "今日视图"}
                        </button>
                        <button
                            onClick={onClose}
                            className="ml-1 w-9 h-9 flex items-center justify-center hover:bg-gray-100 rounded-xl text-gray-400 hover:text-gray-600 transition-colors"
                        >
                            <X size={18} />
                        </button>
                    </div>
                </div>

                {/* Content */}
                <div className="flex-1 overflow-hidden relative bg-gray-50/30">
                    {loading ? (
                        <div className="absolute inset-0 flex flex-col items-center justify-center gap-3">
                            <Loader2 className="animate-spin text-blue-500" size={36} />
                            <span className="text-gray-400 text-sm">加载中...</span>
                        </div>
                    ) : error ? (
                        <div className="absolute inset-0 flex flex-col items-center justify-center gap-3">
                            <span className="text-red-400 font-medium">{error}</span>
                            <button onClick={fetchRoster} className="flex items-center gap-2 text-blue-500 hover:text-blue-600 font-medium">
                                <RefreshCw size={16} />
                                重试
                            </button>
                        </div>
                    ) : isMinimalist ? (
                        renderMinimalistView()
                    ) : (
                        <div className="h-full flex flex-col">
                            <div className="flex-1 overflow-auto p-4">
                                <div className="bg-white rounded-xl border border-gray-200 overflow-hidden shadow-sm">
                                    <table className="w-full border-collapse text-sm">
                                        <thead>
                                            <tr>
                                                {rows[0]?.map((header, idx) => (
                                                    <th key={idx} className="bg-gray-50 border-b border-gray-100 p-3 text-left font-bold text-gray-600 sticky top-0 z-10 min-w-[100px]">
                                                        {header}
                                                    </th>
                                                ))}
                                            </tr>
                                        </thead>
                                        <tbody>
                                            {rows.slice(1).map((row, rIdx) => {
                                                const actualRowIndex = rIdx + 1;
                                                const isReq = isRequirementRow(actualRowIndex);

                                                if (isReq) {
                                                    return (
                                                        <tr key={actualRowIndex} className="bg-amber-50/50">
                                                            <td className="border-b border-gray-100 p-3 font-bold text-amber-600">
                                                                {row[0]?.split(',')[0] || "要求"}
                                                            </td>
                                                            <td colSpan={Math.max(1, (rows[0]?.length || 1) - 1)} className="border-b border-gray-100 p-3 text-amber-800/70 italic">
                                                                {(() => {
                                                                    const content = [];
                                                                    if (row[0] && row[0].includes(',')) content.push(...row[0].split(',').slice(1));
                                                                    for (let c = 1; c < row.length; c++) if (row[c]?.trim()) content.push(row[c].trim());
                                                                    return content.join(' ');
                                                                })()}
                                                            </td>
                                                        </tr>
                                                    );
                                                }

                                                return (
                                                    <tr key={actualRowIndex} className="hover:bg-blue-50/30 transition-colors">
                                                        {row.map((cell, cIdx) => (
                                                            <td key={cIdx} className="border-b border-gray-100 p-0">
                                                                <input
                                                                    className="w-full h-full bg-transparent border-none outline-none p-3 text-gray-700 focus:bg-blue-50 transition-colors"
                                                                    value={cell}
                                                                    onChange={(e) => handleCellChange(actualRowIndex, cIdx, e.target.value)}
                                                                    onBlur={handleSave}
                                                                />
                                                            </td>
                                                        ))}
                                                    </tr>
                                                );
                                            })}
                                        </tbody>
                                    </table>
                                </div>
                            </div>
                            <div className="h-10 bg-white border-t border-gray-100 flex items-center px-4 text-xs text-gray-400">
                                {saving ? (
                                    <span className="flex items-center gap-2 text-blue-500">
                                        <Loader2 size={12} className="animate-spin" />
                                        保存中...
                                    </span>
                                ) : (
                                    <span>所有更改将自动保存</span>
                                )}
                            </div>
                        </div>
                    )}
                </div>
            </div>
        </div>
    );
};

export default DutyRosterModal;
