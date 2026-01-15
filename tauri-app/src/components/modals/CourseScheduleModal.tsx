import { useRef, useState, useEffect, Fragment } from 'react';
import { invoke } from '@tauri-apps/api/core';
import { X, Calendar, Download, RefreshCw, FileSpreadsheet } from 'lucide-react';
import * as XLSX from 'xlsx';
import { useDraggable } from '../../hooks/useDraggable';

interface CourseScheduleModalProps {
    isOpen: boolean;
    onClose: () => void;
    classId?: string;
}

// Mock Schedule Data removed

const CURRENT_TERM = "2023-2024 第二学期";

const CourseScheduleModal = ({ isOpen, onClose, classId }: CourseScheduleModalProps) => {
    const { style, handleMouseDown } = useDraggable();
    const [scheduleData, setScheduleData] = useState<any[]>([]); // Initialize empty
    const [loading, setLoading] = useState(false);
    const fileInputRef = useRef<HTMLInputElement>(null);

    useEffect(() => {
        if (isOpen && classId) {
            fetchSchedule();
        }
    }, [isOpen, classId]);

    const fetchSchedule = async () => {
        if (!classId) return;
        setLoading(true);
        try {
            // Get token
            const userInfoStr = localStorage.getItem('user_info');
            let token = "";
            if (userInfoStr) {
                try {
                    const u = JSON.parse(userInfoStr);
                    token = u.token || "";
                } catch (e) {
                    console.error("Failed to parse user info", e);
                }
            }

            const res = await invoke<string>('get_course_schedule', {
                classId,
                term: CURRENT_TERM,
                token
            });
            console.log("Course Schedule Response:", res);
            try {
                const parsed = JSON.parse(res);
                if (parsed.code === 200 && parsed.data) {
                    // Check for new structure { schedule: ..., cells: ... }
                    if (parsed.data.schedule && parsed.data.cells) {
                        const { times, days } = parsed.data.schedule;
                        const cells = parsed.data.cells;

                        // Reconstruct rows
                        // Reconstruct rows
                        // times is ["08:00..", ..]
                        const rows = times.map((t: string, i: number) => {
                            const row: any = { time: t };

                            // cells have row_index, col_index
                            // days array is ["mon", "tue", "wed", "thu", "fri", "sat", "sun"]

                            days.forEach((d: string, j: number) => {
                                // Find cell by indices matching row i and col j
                                const cell = cells.find((c: any) => c.row_index === i && c.col_index === j);
                                row[d] = cell ? (cell.course_name || cell.subject || "") : "";
                            });

                            // Label handling (lunch)
                            // Robust check for "lunch" or "午"
                            if (t && (t.includes("午") || t.toLowerCase().includes("lunch"))) {
                                row.label = t;
                                row.time = "lunch";
                            }

                            return row;
                        });
                        setScheduleData(rows);
                    } else {
                        // Old fallback
                        const list = Array.isArray(parsed.data) ? parsed.data : (parsed.data.courses || []);
                        if (Array.isArray(list)) {
                            setScheduleData(list);
                        }
                    }
                }
            } catch (e) {
                console.error("Parse error", e);
            }
        } catch (e) {
            console.error("Fetch schedule error:", e);
        } finally {
            setLoading(false);
        }
    };

    const handleImportClick = () => {
        fileInputRef.current?.click();
    };

    const handleFileChange = async (e: React.ChangeEvent<HTMLInputElement>) => {
        const file = e.target.files?.[0];
        if (!file || !classId) return;

        try {
            setLoading(true);
            const data = await file.arrayBuffer();
            const workbook = XLSX.read(data);
            const worksheet = workbook.Sheets[workbook.SheetNames[0]];
            // Use header:1 to get array of arrays (index based)
            // This is robust against mismatched header strings
            const jsonData = XLSX.utils.sheet_to_json(worksheet, { header: 1 }) as any[][];

            // Filter logic:
            // 1. Skip completely empty rows
            // 2. Skip header row (heuristic: contains "周一" or "Mon")
            const mappedData = jsonData
                .filter(row => row && row.length > 0)
                .filter(row => {
                    const str = JSON.stringify(row);
                    return !str.includes("周一") && !str.includes("Mon");
                })
                .map((row) => ({
                    time: row[0] ? String(row[0]) : '', // Col 0 is Time
                    label: (row[0] && String(row[0]).includes('午')) ? String(row[0]) : '', // Heuristic for lunch
                    mon: row[1] ? String(row[1]) : '',
                    tue: row[2] ? String(row[2]) : '',
                    wed: row[3] ? String(row[3]) : '',
                    thu: row[4] ? String(row[4]) : '',
                    fri: row[5] ? String(row[5]) : '',
                    sat: row[6] ? String(row[6]) : '',
                    sun: row[7] ? String(row[7]) : ''
                }));

            // Save to backend
            const userInfoStr = localStorage.getItem('user_info');
            let token = "";
            if (userInfoStr) {
                try {
                    const u = JSON.parse(userInfoStr);
                    token = u.token || "";
                } catch (e) { }
            }

            // Derive metadata
            const times = mappedData.map((d: any) => d.time || "");
            const days = ['mon', 'tue', 'wed', 'thu', 'fri', 'sat', 'sun'];

            // Convert rows to flat cells
            const cells: any[] = [];
            mappedData.forEach((row: any, rowIndex: number) => {
                days.forEach((day, colIndex) => {
                    if (row[day]) {
                        cells.push({
                            row_index: rowIndex, // Send index
                            col_index: colIndex,
                            course_name: row[day], // specific field name
                            day: day, // keep for debug/robustness?
                            time: row.time
                        });
                    }
                });
            });

            const saveRes = await invoke<string>('save_course_schedule', {
                classId,
                term: CURRENT_TERM,
                days,
                times,
                cells: cells,
                token
            });
            console.log("Save Course Schedule Response:", saveRes);

            setScheduleData(mappedData);
            alert("导入并同步成功！");

            fetchSchedule();

        } catch (e) {
            console.error("Import failed", e);
            alert("导入失败");
        } finally {
            setLoading(false);
            if (fileInputRef.current) fileInputRef.current.value = '';
        }
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-200">
            <div
                style={style}
                className="bg-white rounded-2xl shadow-2xl w-[900px] h-[650px] flex flex-col overflow-hidden border border-gray-100"
            >
                {/* Header */}
                <div
                    onMouseDown={handleMouseDown}
                    className="bg-gradient-to-r from-blue-500 to-indigo-600 p-4 flex items-center justify-between text-white flex-shrink-0 cursor-move select-none"
                >
                    <div className="flex items-center gap-2 font-bold text-lg">
                        <Calendar size={20} />
                        <span>班级课程表</span>
                        <span className="text-sm font-normal opacity-80 bg-white/20 px-2 py-0.5 rounded ml-2">{CURRENT_TERM}</span>
                    </div>
                    <div className="flex items-center gap-2">
                        <input
                            type="file"
                            ref={fileInputRef}
                            className="hidden"
                            accept=".xlsx, .xls"
                            onChange={handleFileChange}
                        />
                        <button onClick={handleImportClick} className="p-1.5 hover:bg-white/20 rounded-lg transition-colors text-white/90" title="导入Excel">
                            <FileSpreadsheet size={18} />
                        </button>
                        <button onClick={fetchSchedule} className="p-1.5 hover:bg-white/20 rounded-lg transition-colors text-white/90">
                            <RefreshCw size={18} className={loading ? "animate-spin" : ""} />
                        </button>
                        <button className="p-1.5 hover:bg-white/20 rounded-lg transition-colors text-white/90">
                            <Download size={18} />
                        </button>
                        <div className="w-[1px] h-6 bg-white/30 mx-1"></div>
                        <button onClick={onClose} className="p-1 hover:bg-white/20 rounded-full transition-colors">
                            <X size={20} />
                        </button>
                    </div>
                </div>

                {/* Content */}
                <div className="flex-1 overflow-auto p-4 bg-gray-50/50">
                    {scheduleData.length === 0 ? (
                        /* Empty State */
                        <div className="h-full flex flex-col items-center justify-center text-center">
                            <Calendar size={64} className="text-gray-300 mb-4" />
                            <h3 className="text-lg font-bold text-gray-500 mb-2">暂无课程表数据</h3>
                            <p className="text-gray-400 text-sm mb-6">请导入Excel课程表文件</p>
                            <button
                                onClick={handleImportClick}
                                className="px-6 py-3 bg-blue-600 hover:bg-blue-700 text-white font-bold rounded-lg flex items-center gap-2 transition-all shadow-lg shadow-blue-200"
                            >
                                <FileSpreadsheet size={20} />
                                导入课程表
                            </button>
                        </div>
                    ) : (
                        <div className="bg-white rounded-xl border border-gray-200 shadow-sm overflow-hidden">
                            <div className="grid grid-cols-6 text-sm">
                                {/* Header Row */}
                                <div className="bg-gray-50 p-3 font-bold text-gray-500 text-center border-b border-r border-gray-200">时间</div>
                                {['周一', '周二', '周三', '周四', '周五'].map(day => (
                                    <div key={day} className="bg-gray-50 p-3 font-bold text-gray-700 text-center border-b border-gray-200 border-r last:border-r-0">
                                        {day}
                                    </div>
                                ))}

                                {/* Schedule Rows */}
                                {scheduleData.map((row, index) => {
                                    if (row.time === 'lunch') {
                                        return (
                                            <div key={index} className="col-span-6 bg-orange-50/50 p-2 text-center text-orange-400 font-medium text-xs border-b border-gray-200">
                                                {row.label}
                                            </div>
                                        );
                                    }
                                    return (
                                        <Fragment key={index}>
                                            {/* Time Column */}
                                            <div className="p-4 flex items-center justify-center text-gray-500 font-medium border-b border-r border-gray-100 text-xs bg-gray-50/30">
                                                {row.time}
                                            </div>
                                            {/* Course Columns */}
                                            {[row.mon, row.tue, row.wed, row.thu, row.fri].map((course, i) => (
                                                <div key={i} className="p-2 border-b border-r border-gray-100 last:border-r-0 h-20 relative group hover:bg-blue-50/50 transition-colors">
                                                    <div className={`
                                                    h-full w-full rounded-lg flex flex-col items-center justify-center gap-1 cursor-pointer transition-all hover:shadow-sm
                                                    ${course === '数学' ? 'bg-blue-100 text-blue-700' :
                                                            course === '语文' ? 'bg-red-100 text-red-700' :
                                                                course === '英语' ? 'bg-orange-100 text-orange-700' :
                                                                    course === '物理' ? 'bg-indigo-100 text-indigo-700' :
                                                                        course === '化学' ? 'bg-purple-100 text-purple-700' :
                                                                            course === '体育' ? 'bg-green-100 text-green-700' :
                                                                                'bg-gray-100 text-gray-600'}
                                                `}>
                                                        <span className="font-bold">{course || '-'}</span>
                                                        {course && <span className="text-[10px] opacity-70">A-302</span>}
                                                    </div>
                                                </div>
                                            ))}
                                        </Fragment>
                                    );
                                })}
                            </div>
                        </div>
                    )}
                </div>
            </div>
        </div>
    );
};

export default CourseScheduleModal;
