import { useState, useEffect } from 'react';
import * as XLSX from 'xlsx';
import { X } from 'lucide-react';

interface StudentPhysiqueModalProps {
    isOpen: boolean;
    onClose: () => void;
    fileName: string;
    data: any[]; // Array of Arrays
    classId?: string;
    file?: File;
    autoPrompt?: boolean;
    onAutoPromptComplete?: () => void;
    onDataChange?: (newData: any[]) => void;
}

interface TableRow {
    _id: number;
    [key: string]: any;
}

const StudentPhysiqueModal = ({ isOpen, onClose, fileName, data, classId, file, autoPrompt, onAutoPromptComplete, onDataChange }: StudentPhysiqueModalProps) => {
    const [headers, setHeaders] = useState<string[]>([]);
    const [rows, setRows] = useState<TableRow[]>([]);
    const [selectedRowId, setSelectedRowId] = useState<number | null>(null);

    const [selectedCol, setSelectedCol] = useState<string | null>(null);

    // Helper to sync with parent
    const notifyParent = (currentRows: TableRow[], currentHeaders: string[]) => {
        if (!onDataChange) return;
        const aoa = [
            currentHeaders,
            ...currentRows.map(r => {
                const rowArr: any = currentHeaders.map(h => r[h]);
                // Re-attach comment properties
                Object.keys(r).forEach(k => {
                    if (k.startsWith('_comment_')) {
                        rowArr[k] = r[k];
                    }
                });
                return rowArr;
            })
        ];
        onDataChange(aoa);
    };

    // Recalculate Totals (Individual + Group) for ALL rows (safest for correctness)
    const recalculateAllTotals = (currentRows: TableRow[], currentHeaders: string[]) => {
        // Helper to round to 1 decimal place (typical for school grades)
        const round = (n: number) => Math.round(n * 10) / 10;

        // 1. Calculate Individual Totals
        const rowsWithIndTotal = currentRows.map(r => {
            let sum = 0;
            currentHeaders.forEach(h => {
                if (!['学号', '姓名', '小组', '总分', '小组总分'].includes(h)) {
                    const val = parseFloat(r[h]);
                    if (!isNaN(val)) sum += val;
                }
            });
            return { ...r, '总分': round(sum) } as TableRow;
        });

        // 2. Calculate Group Totals
        // Map groupName -> Total Score
        const groupTotals: Record<string, number> = {};
        rowsWithIndTotal.forEach(r => {
            const g = r['小组'];
            if (g) {
                groupTotals[g] = (groupTotals[g] || 0) + (r['总分'] || 0);
            }
        });

        // 3. Apply Group Totals (also rounded)
        return rowsWithIndTotal.map(r => {
            const g = r['小组'];
            if (g && groupTotals[g] !== undefined) {
                return { ...r, '小组总分': round(groupTotals[g]) };
            }
            return { ...r, '小组总分': 0 };
        });
    };

    useEffect(() => {
        if (data && data.length > 0) {
            const rawHeaders = (data[0] as any[]).map(h => String(h).trim());
            console.log("Physique Raw Headers:", rawHeaders);

            let processedHeaders = [...rawHeaders];
            if (!processedHeaders.includes('总分')) processedHeaders.push('总分');
            if (!processedHeaders.includes('小组总分')) processedHeaders.push('小组总分');

            // Process rows
            // Logic to fill empty group names with previous row's group if empty (common in excel)
            let lastGroup = "";
            const groupColIdx = rawHeaders.indexOf('小组');

            let processedRows = data.slice(1).map((row, idx) => {
                const rowObj: TableRow = { _id: idx };

                // Handle Group Fill
                let groupVal = groupColIdx >= 0 ? row[groupColIdx] : "";
                if (groupVal) {
                    lastGroup = groupVal;
                } else if (lastGroup && (!groupVal || groupVal.trim() === '')) {
                    // If current group is empty but we have a last group, fill it? 
                    // QT logic: "if group exists... if empty use currentGroup".
                    rowObj['小组'] = lastGroup;
                }

                rawHeaders.forEach((h, i) => {
                    if (h === '小组' && rowObj['小组']) return; // Already set
                    rowObj[h] = row[i];
                });

                // Copy any comment fields attached to the row array object
                Object.keys(row).forEach(key => {
                    if (key.startsWith('_comment_')) {
                        rowObj[key] = (row as any)[key];
                    }
                });

                // Ensure '小组' is set in rowObj if it wasn't in loop
                if (groupColIdx >= 0 && !rowObj['小组'] && lastGroup) {
                    rowObj['小组'] = lastGroup;
                }
                return rowObj;
            });

            // Initial Calculation
            processedRows = recalculateAllTotals(processedRows, processedHeaders);

            setHeaders(processedHeaders);
            setRows(processedRows);
        }
    }, [data]);

    // Auto Prompt for Upload
    useEffect(() => {
        if (isOpen && autoPrompt && rows.length > 0) {
            const timer = setTimeout(() => {
                if (window.confirm("导入成功！是否立即上传到服务器？")) {
                    handleUpload();
                }
                if (onAutoPromptComplete) onAutoPromptComplete();
            }, 300);
            return () => clearTimeout(timer);
        }
    }, [isOpen, autoPrompt, rows, onAutoPromptComplete]);

    const calculateTerm = () => {
        const now = new Date();
        const year = now.getFullYear();
        const month = now.getMonth() + 1; // 1-12
        if (month >= 9 || month <= 1) {
            const startYear = month >= 9 ? year : year - 1;
            return `${startYear}-${startYear + 1}-1`;
        } else {
            return `${year - 1}-${year}-2`;
        }
    };

    const handleUpload = async () => {
        let targetClassId = classId;
        if (!targetClassId) {
            // Fallback: try to get from URL
            const match = window.location.pathname.match(/\/class\/(?:chat|schedule)\/([^/]+)/);
            if (match && match[1]) {
                targetClassId = match[1];
                console.log("Recovered classId from URL:", targetClassId);
            }
        }

        if (!targetClassId) {
            alert(`缺少班级ID，无法上传！\n当前路径: ${window.location.pathname}`);
            return;
        }
        // Fix Class ID: Remove suffix '01' if present
        if (targetClassId && targetClassId.length === 11) {
            targetClassId = targetClassId.substring(0, 9);
        }

        const term = calculateTerm();

        // Data payload construction for Group Scores (Nested structure per QT)
        const groupMap = new Map<string, { groupName: string, groupTotal: number, students: any[] }>();

        rows.forEach(r => {
            const gName = String(r['小组'] || "").trim(); // Ensure string
            if (!gName) return;

            if (!groupMap.has(gName)) {
                groupMap.set(gName, {
                    groupName: gName,
                    groupTotal: r['小组总分'] || 0,
                    students: []
                });
            }

            const groupEntry = groupMap.get(gName)!;

            // Construct scores object (dynamic fields)
            const scoresMap: Record<string, any> = {};
            headers.forEach(h => {
                if (!['小组', '学号', '姓名', '小组总分'].includes(h)) {
                    // Include '总分' as a score field in 'scores' map if present in QT log?
                    // QT log: scores: { "作业": 37, "总分": 258.6, ... } -> YES, Total is inside scores.
                    const val = r[h];
                    if (val !== undefined && val !== "") {
                        scoresMap[h] = parseFloat(val) || 0; // QT sends numbers for scores
                    }
                }
            });

            groupEntry.students.push({
                student_id: r['学号'],
                student_name: r['姓名'],
                scores: scoresMap
            });
        });

        const groupScoresPayload = Array.from(groupMap.values()).map(g => ({
            group_name: g.groupName,
            group_total_score: g.groupTotal,
            students: g.students
        }));

        const fieldStrings = headers.filter(h => !['小组', '学号', '姓名'].includes(h));

        // Convert to objects to avoid server 500 error: 'str' object has no attribute 'get'
        const fields = fieldStrings.map((h, idx) => ({
            field_name: h,
            field_type: "number",
            field_order: idx + 1,
            is_total: ['总分', '小组总分'].includes(h) ? 1 : 0
        }));

        const description = `说明:该表为统计表。包含以下科目/属性: ${fieldStrings.filter(f => !['总分', '小组总分'].includes(f)).join('、')}`;

        const payload = {
            class_id: targetClassId,
            exam_name: fileName,
            remark: description,
            excel_file_description: description,
            term: term,
            operation_mode: "replace",
            excel_file_name: file?.name || fileName,
            fields: fields, // Objects
            excel_files: [
                {
                    filename: file?.name || fileName,
                    description: description,
                    url: "",
                    fields: fieldStrings // Strings
                }
            ],
            group_scores: groupScoresPayload
        };
        console.log("Upload Payload:", JSON.stringify(payload, null, 2));

        const formData = new FormData();
        formData.append('data', JSON.stringify(payload));
        if (file) {
            formData.append('excel_file', file);
        }

        try {
            const response = await fetch('/api/group-scores/save', {
                method: 'POST',
                body: formData
            });
            const resData = await response.json();
            if (resData.code === 200) {
                alert(`上传成功！\n${resData.message}`);
            } else {
                alert(`上传失败: ${resData.message}`);
            }
        } catch (error) {
            console.error("Upload error:", error);
            alert("上传出错，请检查网络或服务器状态");
        }
    };

    // Calculate Row Spans for Group Column
    const getGroupRowSpan = (rowIndex: number): number => {
        const currentRow = rows[rowIndex];
        const currentGroup = currentRow['小组'];
        if (!currentGroup) return 1;

        // Check if previous row has same group
        if (rowIndex > 0 && rows[rowIndex - 1]['小组'] === currentGroup) {
            return 0; // Already spanned
        }

        // Count following rows
        let span = 1;
        for (let i = rowIndex + 1; i < rows.length; i++) {
            if (rows[i]['小组'] === currentGroup) {
                span++;
            } else {
                break;
            }
        }
        return span;
    };

    const [contextMenu, setContextMenu] = useState<{ x: number, y: number, rowId: number, colKey: string } | null>(null);

    // Close context menu on click elsewhere
    useEffect(() => {
        const handleClick = () => setContextMenu(null);
        window.addEventListener('click', handleClick);
        return () => window.removeEventListener('click', handleClick);
    }, []);

    const setScoreToServer = async (rowId: number, colKey: string, newValue: string, studentName: string, studentId: string) => {
        let cleanClassId = classId;
        if (!cleanClassId) {
            const match = window.location.pathname.match(/\/class\/(?:chat|schedule)\/([^/]+)/);
            if (match && match[1]) cleanClassId = match[1];
        }
        if (cleanClassId && cleanClassId.length === 11) cleanClassId = cleanClassId.substring(0, 9);

        if (!cleanClassId) return;

        const scoreVal = newValue.trim() === "" ? null : parseFloat(newValue);
        if (scoreVal !== null && isNaN(scoreVal)) {
            alert("必须输入数字 (或留空表示删除)!");
            return;
        }

        const payload = {
            class_id: cleanClassId,
            term: calculateTerm(),
            student_name: studentName,
            student_id: studentId,
            field_name: colKey,
            excel_filename: fileName,
            score: scoreVal
        };

        try {
            const res = await fetch('/api/group-scores/set-score', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload)
            });
            const data = await res.json();
            if (data.code === 200) {
                // Server might return updated totals. 
                // For now, we trust our local recalculateAllTotals which is already called.
                // But if server returns specific "excel_total" we could use it.
            } else {
                alert(`设置分数失败: ${data.message}`);
            }
        } catch (e) {
            console.error("Set Score Error:", e);
        }
    };

    const setCommentToServer = async (rowId: number, colKey: string, comment: string, studentName: string, studentId: string) => {
        let cleanClassId = classId;
        if (!cleanClassId) {
            const match = window.location.pathname.match(/\/class\/(?:chat|schedule)\/([^/]+)/);
            if (match && match[1]) cleanClassId = match[1];
        }
        if (cleanClassId && cleanClassId.length === 11) cleanClassId = cleanClassId.substring(0, 9);

        const payload = {
            class_id: cleanClassId,
            term: calculateTerm(),
            student_name: studentName,
            student_id: studentId,
            field_name: colKey,
            excel_filename: fileName,
            comment: comment
        };

        try {
            const res = await fetch('/api/group-scores/set-comment', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload)
            });
            const data = await res.json();
            if (data.code === 200) {
                setRows(prev => {
                    const newRows = prev.map(r => {
                        if (r._id === rowId) {
                            return { ...r, [`_comment_${colKey}`]: comment };
                        }
                        return r;
                    });
                    notifyParent(newRows, headers);
                    return newRows;
                });
            } else {
                alert(`设置注释失败: ${data.message}`);
            }
        } catch (e) {
            console.error("Set Comment Error:", e);
        }
    };

    const handleContextMenu = (e: React.MouseEvent, rowId: number, colKey: string) => {
        e.preventDefault();
        setContextMenu({ x: e.pageX, y: e.pageY, rowId, colKey });
    };

    const handleAddComment = () => {
        if (!contextMenu) return;
        const { rowId, colKey } = contextMenu;
        const row = rows.find(r => r._id === rowId);
        if (!row) return;

        const currentComment = row[`_comment_${colKey}`] || "";
        const newComment = prompt("请输入注释:", currentComment);

        if (newComment !== null) {
            setCommentToServer(rowId, colKey, newComment, row['姓名'], row['学号']);
        }
        setContextMenu(null);
    };

    const handleValueClick = (rowId: number, colKey: string, currentValue: any, studentName: string, studentId: string) => {
        if (['总分', '小组总分', '小组', '姓名', '学号'].includes(colKey)) return;

        const newValue = prompt(`请输入 ${colKey} 的新值:`, currentValue || '');
        if (newValue !== null) {
            setRows(prev => {
                const updatedRows = prev.map(r => {
                    if (r._id === rowId) {
                        return { ...r, [colKey]: newValue };
                    }
                    return r;
                });
                const recalculated = recalculateAllTotals(updatedRows, headers);
                notifyParent(recalculated, headers);
                return recalculated;
            });

            setScoreToServer(rowId, colKey, newValue, studentName, studentId);
        }
    };

    const handleAddRow = () => {
        const newId = rows.length > 0 ? Math.max(...rows.map(r => r._id)) + 1 : 0;
        // Try to inherit group from selected row or last row
        const lastRow = rows[rows.length - 1];
        const newRow: TableRow = { _id: newId, '学号': '', '姓名': '', '小组': lastRow?.['小组'] || '', '总分': 0, '小组总分': 0 };
        headers.forEach(h => {
            if (newRow[h] === undefined) newRow[h] = "";
        });
        setRows(prev => {
            const newRows = recalculateAllTotals([...prev, newRow], headers);
            notifyParent(newRows, headers);
            return newRows;
        });
    };

    const handleDeleteRow = () => {
        if (selectedRowId === null) {
            if (confirm("未选择行，是否删除最后一行？")) {
                setRows(prev => {
                    if (prev.length === 0) return prev;
                    const newRows = recalculateAllTotals(prev.slice(0, prev.length - 1), headers);
                    notifyParent(newRows, headers);
                    return newRows;
                });
            }
            return;
        }
        if (confirm("确定删除选中行吗？")) {
            setRows(prev => {
                const newRows = recalculateAllTotals(prev.filter(r => r._id !== selectedRowId), headers);
                notifyParent(newRows, headers);
                return newRows;
            });
            setSelectedRowId(null);
        }
    };

    const handleAddColumn = () => {
        const name = prompt("请输入新列名:");
        if (name && !headers.includes(name)) {
            const newHeaders = [...headers];
            // Insert before '总分'
            const idx = newHeaders.indexOf('总分');
            if (idx > -1) newHeaders.splice(idx, 0, name);
            else newHeaders.push(name);

            setHeaders(newHeaders);
            setRows(prev => {
                const newRows = prev.map(r => ({ ...r, [name]: '' }));
                notifyParent(newRows, newHeaders);
                return newRows;
            });
        } else if (name) {
            alert("列名已存在!");
        }
    };

    const handleDeleteColumn = () => {
        const colName = selectedCol || prompt("请输入要删除的列名:");
        if (!colName) return;
        if (['学号', '姓名', '小组', '总分', '小组总分'].includes(colName)) {
            alert("该列不可删除!");
            return;
        }
        if (headers.includes(colName)) {
            if (confirm(`确定删除列 "${colName}" 吗？`)) {
                const newHeaders = headers.filter(h => h !== colName);
                setHeaders(newHeaders);
                setRows(prev => {
                    const newRows = recalculateAllTotals(prev.map(r => {
                        const newR = { ...r };
                        delete newR[colName];
                        return newR;
                    }), newHeaders);
                    notifyParent(newRows, newHeaders);
                    return newRows;
                });
                setSelectedCol(null);
            }
        }
    };

    const handleExport = () => {
        const aoa = [
            headers,
            ...rows.map(r => headers.map(h => r[h]))
        ];
        const ws = XLSX.utils.aoa_to_sheet(aoa);
        const wb = XLSX.utils.book_new();
        XLSX.utils.book_append_sheet(wb, ws, "Sheet1");
        XLSX.writeFile(wb, `${fileName || "GroupExport"}.xlsx`);
    };

    if (!isOpen) return null;

    return (
        <div className="fixed inset-0 z-[70] flex items-center justify-center bg-black/40 backdrop-blur-sm animate-in fade-in duration-200">
            {/* Context Menu */}
            {contextMenu && (
                <div
                    className="fixed bg-white border border-gray-300 shadow-lg rounded py-1 z-[80]"
                    style={{ top: contextMenu.y, left: contextMenu.x }}
                    onClick={(e) => e.stopPropagation()}
                >
                    <button
                        className="block w-full text-left px-4 py-2 hover:bg-gray-100 text-sm"
                        onClick={handleAddComment}
                    >
                        编辑注释
                    </button>
                </div>
            )}

            <div className="bg-white rounded-2xl shadow-2xl w-[1200px] h-[800px] flex flex-col overflow-hidden text-sm select-none relative border border-gray-100">

                {/* Close */}
                <button
                    onClick={onClose}
                    className="absolute top-4 right-4 w-8 h-8 flex items-center justify-center text-gray-400 hover:text-gray-600 hover:bg-gray-100 rounded-lg transition-all z-20"
                >
                    ×
                </button>

                {/* Header */}
                <div className="p-6 pb-4 flex flex-col items-center justify-center gap-2 border-b border-gray-100 bg-gradient-to-r from-gray-50 to-white">
                    <div className="flex items-center gap-3">
                        <div className="w-10 h-10 rounded-xl bg-gradient-to-br from-blue-400 to-indigo-500 flex items-center justify-center shadow-lg">
                            <span className="text-white text-lg">👥</span>
                        </div>
                        <div>
                            <h2 className="text-gray-800 font-bold text-lg">{fileName}</h2>
                            <p className="text-xs text-gray-500">小组评价表 · 包含 {headers.filter(h => !['小组', '学号', '姓名', '总分', '小组总分'].includes(h)).length} 个评分项</p>
                        </div>
                    </div>
                </div>

                {/* Toolbar */}
                <div className="px-6 py-3 flex gap-2 overflow-x-auto bg-gray-50/50 border-b border-gray-100">
                    <button onClick={handleAddRow} className="px-4 py-2 rounded-lg text-gray-700 text-xs font-medium whitespace-nowrap bg-white border border-gray-200 hover:border-blue-300 hover:text-blue-600 hover:shadow-sm transition-all">
                        + 添加行
                    </button>
                    <button onClick={handleDeleteRow} className={`px-4 py-2 rounded-lg text-xs font-medium whitespace-nowrap transition-all ${selectedRowId !== null ? 'bg-red-50 text-red-600 border border-red-200 hover:bg-red-100' : 'bg-white text-gray-700 border border-gray-200 hover:border-gray-300'}`}>
                        {selectedRowId !== null ? '🗑 删除选中行' : '删除行'}
                    </button>
                    <button onClick={handleDeleteColumn} className={`px-4 py-2 rounded-lg text-xs font-medium whitespace-nowrap transition-all ${selectedCol ? 'bg-red-50 text-red-600 border border-red-200 hover:bg-red-100' : 'bg-white text-gray-700 border border-gray-200 hover:border-gray-300'}`}>
                        {selectedCol ? `🗑 删除列(${selectedCol})` : '删除列'}
                    </button>
                    <button onClick={handleAddColumn} className="px-4 py-2 rounded-lg text-gray-700 text-xs font-medium whitespace-nowrap bg-white border border-gray-200 hover:border-blue-300 hover:text-blue-600 hover:shadow-sm transition-all">
                        + 添加列
                    </button>
                    <div className="flex-1" />
                    <button onClick={handleExport} className="px-4 py-2 rounded-lg text-gray-700 text-xs font-medium whitespace-nowrap bg-white border border-gray-200 hover:border-gray-300 hover:shadow-sm transition-all">
                        📥 导出
                    </button>
                    <button onClick={handleUpload} className="px-4 py-2 rounded-lg text-white text-xs font-medium whitespace-nowrap bg-gradient-to-r from-blue-500 to-indigo-500 hover:from-blue-600 hover:to-indigo-600 shadow-sm hover:shadow-md transition-all">
                        ☁️ 上传服务器
                    </button>
                </div>

                {/* Table Area */}
                <div className="flex-1 overflow-auto mx-6 my-4 rounded-xl border border-gray-200 bg-white shadow-inner">
                    <table className="w-full border-collapse">
                        <thead className="bg-gray-50 sticky top-0 z-10">
                            <tr>
                                <th className="w-10 border-b border-r border-gray-200 p-2 bg-gray-50 text-gray-400 text-xs font-medium">#</th>
                                {headers.map(h => (
                                    <th
                                        key={h}
                                        onClick={() => !['学号', '姓名', '小组', '总分', '小组总分'].includes(h) && setSelectedCol(h === selectedCol ? null : h)}
                                        className={`border-b border-r border-gray-200 p-3 text-gray-700 min-w-[80px] font-semibold text-xs uppercase tracking-wide cursor-pointer transition-all ${selectedCol === h
                                                ? 'bg-blue-50 text-blue-600'
                                                : 'bg-gray-50 hover:bg-gray-100'
                                            } ${['总分', '小组总分'].includes(h) ? 'bg-indigo-50 text-indigo-700' : ''}`}
                                    >
                                        {h}
                                    </th>
                                ))}
                            </tr>
                        </thead>
                        <tbody>
                            {rows.map((row, idx) => {
                                const groupSpan = getGroupRowSpan(idx);

                                return (
                                    <tr
                                        key={row._id}
                                        onClick={() => setSelectedRowId(row._id)}
                                        className={`cursor-pointer transition-colors ${selectedRowId === row._id ? 'bg-blue-50' : 'hover:bg-gray-50'}`}
                                    >
                                        <td className="border-b border-r border-gray-100 bg-gray-50/50 text-center text-gray-400 text-xs font-mono select-none p-2">{idx + 1}</td>

                                        {headers.map(col => {
                                            const val = row[col];

                                            // Handle Group Column Merge
                                            if (col === '小组') {
                                                if (groupSpan === 0) return null;
                                                return (
                                                    <td key={col} rowSpan={groupSpan} className="border-b border-r border-gray-200 p-2 text-center bg-gradient-to-r from-blue-50 to-indigo-50 align-middle font-bold text-indigo-700">
                                                        {val}
                                                    </td>
                                                );
                                            }

                                            const isFixed = ['学号', '姓名', '小组'].includes(col);
                                            const isTotal = ['总分', '小组总分'].includes(col);
                                            const isScore = !isFixed && !isTotal;
                                            const hasComment = row[`_comment_${col}`] ? true : false;

                                            return (
                                                <td
                                                    key={col}
                                                    className={`border-b border-r border-gray-100 p-2 text-center ${selectedCol === col ? 'bg-blue-50/50' : ''
                                                        } ${hasComment ? 'bg-yellow-50' : ''} ${isTotal ? 'bg-indigo-50/30' : ''
                                                        }`}
                                                    onContextMenu={(e) => {
                                                        if (isScore) handleContextMenu(e, row._id, col);
                                                    }}
                                                >
                                                    {isScore ? (
                                                        <button
                                                            onClick={(e) => { e.stopPropagation(); setSelectedRowId(row._id); handleValueClick(row._id, col, val, row['姓名'], row['学号']); }}
                                                            className={`w-full h-full px-2 py-1 text-gray-700 hover:bg-gray-100 rounded text-sm text-center transition-colors ${hasComment ? 'ring-2 ring-yellow-300 ring-inset' : ''}`}
                                                            title={row[`_comment_${col}`]}
                                                        >
                                                            {val || ''}
                                                        </button>
                                                    ) : isTotal ? (
                                                        <span className="text-indigo-600 font-bold px-2">
                                                            {val || '0'}
                                                        </span>
                                                    ) : (
                                                        <span className="text-gray-800 font-medium">{val}</span>
                                                    )}
                                                </td>
                                            );
                                        })}
                                    </tr>
                                );
                            })}
                        </tbody>
                    </table>
                </div>

            </div>
        </div>
    );
};
export default StudentPhysiqueModal;
