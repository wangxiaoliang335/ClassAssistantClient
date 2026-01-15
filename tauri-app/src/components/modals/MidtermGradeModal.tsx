import { useState, useEffect } from 'react';
import * as XLSX from 'xlsx';

interface MidtermGradeModalProps {
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
    _id: number; // Internal ID for React keys
    [key: string]: any;
}

const MidtermGradeModal = ({ isOpen, onClose, fileName, data, classId, file, autoPrompt, onAutoPromptComplete, onDataChange }: MidtermGradeModalProps) => {
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
                // Re-attach comment properties to the array object
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

    useEffect(() => {
        if (data && data.length > 0) {
            const rawHeaders = (data[0] as any[]).map(h => String(h).trim());

            // Only re-process if headers or length mismatches to avoid loop if object ref changes but content is same?
            // But 'data' prop WILL change reference on every parent update.
            // We need to break the loop. 
            // The loop is: Parent Data Change -> Prop Data Change -> useEffect -> setRows. 
            // If we notify parent inside the actions, parent updates Data, Prop updates.
            // useEffect will fire again.
            // We can prevent unnecessary setRows if we compare deep equality, but that's expensive.
            // OR, we can use a ref to track if the update came from self?
            // Actually, if we just setRows here, it is fine. The user action triggered the change, parent updated, passed back. 
            // The key is: Does `setRows` cause a re-render/re-effect that triggers `notifyParent`? 
            // NO, `notifyParent` is only called on user actions, not in useEffect.
            // So the Flow: User Action -> setRows + notifyParent -> Parent Update -> Prop Update -> useEffect -> setRows.
            // Ideally currentRows and newPropRows are identical. 
            // We just need to make sure we don't notifyParent inside the useEffect!

            let processedHeaders = [...rawHeaders];
            if (!processedHeaders.includes('总分')) {
                processedHeaders.push('总分');
            }

            let processedRows = data.slice(1).map((row, idx) => {
                const rowObj: TableRow = { _id: idx };
                rawHeaders.forEach((h, i) => {
                    rowObj[h] = row[i];
                });

                Object.keys(row).forEach(key => {
                    if (key.startsWith('_comment_')) {
                        rowObj[key] = (row as any)[key];
                    }
                });
                return rowObj;
            });

            // Auto-calculate Totals upon import
            const round = (n: number) => Math.round(n * 10) / 10;
            processedRows = processedRows.map(r => {
                let sum = 0;
                processedHeaders.forEach(h => {
                    if (!['学号', '姓名', '总分'].includes(h)) {
                        const val = parseFloat(r[h]);
                        if (!isNaN(val)) {
                            sum += val;
                        }
                    }
                });
                return { ...r, '总分': round(sum) };
            });

            setHeaders(processedHeaders);
            setRows(processedRows);
        }
    }, [data]);

    // Auto Prompt for Upload
    useEffect(() => {
        if (isOpen && autoPrompt && rows.length > 0) { // Wait for rows to populate
            const timer = setTimeout(() => {
                if (window.confirm("导入成功！是否立即上传到服务器？")) {
                    handleUpload();
                }
                if (onAutoPromptComplete) onAutoPromptComplete();
            }, 300); // 300ms delay for smooth UI transition
            return () => clearTimeout(timer);
        }
    }, [isOpen, autoPrompt, rows, onAutoPromptComplete]);

    const calculateTerm = () => {
        const now = new Date();
        const year = now.getFullYear();
        const month = now.getMonth() + 1; // 1-12
        // Sep(9) to Jan(1) is first term of (year)-(year+1)
        // Else is second term of (year-1)-(year)
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
            // Supports: /class/chat/:id and /class/schedule/:id
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

        // Fix Class ID: Remove suffix '01' if present (e.g. 00001100601 -> 000011006)
        if (targetClassId && targetClassId.length === 11) {
            targetClassId = targetClassId.substring(0, 9);
        }

        const term = calculateTerm();
        const scores = rows.map(r => {
            const scoreObj: any = {
                student_name: r['姓名'],
                student_id: r['学号'],
            };
            // Add dynamic fields
            headers.forEach(h => {
                if (!['学号', '姓名'].includes(h)) {
                    scoreObj[h] = r[h];
                }
            });
            // Total is already in headers loop if we include it
            // distinct handling for Total is not needed if it's in headers
            // BUT backend might expect 'total_score' key explicitly for the main column
            if (r['总分']) {
                scoreObj['total_score'] = r['总分'];
            }
            return scoreObj;
        });

        const fieldStrings = headers.filter(h => !['学号', '姓名'].includes(h));
        // Server expects objects for fields, based on "str object has no attribute get" error.
        const fields = fieldStrings.map((h, idx) => ({
            field_name: h,
            field_type: "number", // Defaulting to number
            field_order: idx + 1,
            is_total: h === '总分' ? 1 : 0
        }));

        const description = `说明:该表为统计表。包含以下科目/属性: ${fieldStrings.filter(f => f !== '总分').join('、')}`;

        const payload = {
            class_id: targetClassId,
            exam_name: fileName,
            remark: description, // Matches QT 'remark'
            excel_file_description: description, // Matches QT
            term: term,
            operation_mode: "replace",
            excel_file_name: file?.name || fileName,
            fields: fields, // Objects
            excel_files: [
                {
                    filename: file?.name || fileName,
                    description: description,
                    url: "",
                    fields: fieldStrings // Strings: ["总分", "数学", ...] - Note: QT example puts '总分' first usually
                }
            ],
            scores: scores
        };
        console.log("Upload Payload:", JSON.stringify(payload, null, 2));

        const formData = new FormData();
        formData.append('data', JSON.stringify(payload));
        if (file) {
            formData.append('excel_file', file);
        }

        try {
            const response = await fetch('/api/student-scores/save', {
                method: 'POST',
                body: formData
            });
            const resData = await response.json();
            if (resData.code === 200) {
                alert(`上传成功！\n${resData.message}`);
                // onClose(); // Optional: close on success?
            } else {
                alert(`上传失败: ${resData.message}`);
            }
        } catch (error) {
            console.error("Upload error:", error);
            alert("上传出错，请检查网络或服务器状态");
        }
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

        if (!cleanClassId) {
            console.error("Missing Class ID for set-score");
            return;
        }

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
            const res = await fetch('/api/student-scores/set-score', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload)
            });
            const data = await res.json();
            if (data.code === 200) {
                // Update Row with server returned Total Score if available
                const serverData = data.data;
                let newTotal = null;
                if (serverData.excel_total !== undefined && serverData.excel_total !== null) newTotal = serverData.excel_total;
                else if (serverData.excel_total_score !== undefined && serverData.excel_total_score !== null) newTotal = serverData.excel_total_score;
                else if (serverData.total_score !== undefined && serverData.total_score !== null) newTotal = serverData.total_score;

                if (newTotal !== null) {
                    setRows(prev => prev.map(r => {
                        if (r._id === rowId) {
                            return { ...r, '总分': newTotal };
                        }
                        return r;
                    }));
                }
            } else {
                console.error("Set Score Failed:", data.message);
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
            const res = await fetch('/api/student-scores/set-comment', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload)
            });
            const data = await res.json();
            if (data.code === 200) {
                // Update local state to show comment indicator?
                // We can store comments in a separate state or special field in row like `_comment_${colKey}`
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

    const handleValueClick = (rowId: number, colKey: string, currentValue: any, studentName: string, studentId: string) => {
        if (['总分'].includes(colKey)) return;

        const newValue = prompt(`请输入 ${colKey} 的新值:`, currentValue || '');
        if (newValue !== null) {
            // 1. Optimistic Update
            setRows(prev => {
                const updatedRows = prev.map(r => {
                    if (r._id === rowId) {
                        const newRow = { ...r, [colKey]: newValue };
                        newRow['总分'] = calculateRowTotal(newRow, headers);
                        return newRow;
                    }
                    return r;
                });
                notifyParent(updatedRows, headers);
                return updatedRows;
            });

            // 2. Server Update
            setScoreToServer(rowId, colKey, newValue, studentName, studentId);
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
        setContextMenu(null); // Close menu
    };
    // Recalculate Total for a single row object
    const calculateRowTotal = (row: TableRow, currentHeaders: string[]) => {
        let sum = 0;
        currentHeaders.forEach(h => {
            if (!['学号', '姓名', '总分'].includes(h)) {
                const val = parseFloat(row[h]);
                if (!isNaN(val)) {
                    sum += val;
                }
            }
        });
        return sum;
    };

    const handleAddRow = () => {
        const newId = rows.length > 0 ? Math.max(...rows.map(r => r._id)) + 1 : 0;
        const newRow: TableRow = { _id: newId, '学号': '', '姓名': '', '总分': 0 };
        headers.forEach(h => {
            if (!newRow[h]) newRow[h] = "";
        });
        setRows(prev => {
            const newRows = [...prev, newRow];
            notifyParent(newRows, headers);
            return newRows;
        });
    };

    const handleDeleteRow = () => {
        if (selectedRowId === null) {
            if (confirm("未选择行，是否删除最后一行？")) {
                setRows(prev => {
                    if (prev.length === 0) return prev;
                    const newRows = prev.slice(0, prev.length - 1);
                    notifyParent(newRows, headers);
                    return newRows;
                });
            }
            return;
        }
        if (confirm("确定删除选中行吗？")) {
            setRows(prev => {
                const newRows = prev.filter(r => r._id !== selectedRowId);
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
            const totalIdx = newHeaders.indexOf('总分');
            if (totalIdx > -1) {
                newHeaders.splice(totalIdx, 0, name);
            } else {
                newHeaders.push(name);
            }
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
        if (['学号', '姓名', '总分'].includes(colName)) {
            alert("该列不可删除!");
            return;
        }
        if (headers.includes(colName)) {
            if (confirm(`确定删除列 "${colName}" 吗？`)) {
                const newHeaders = headers.filter(h => h !== colName);
                setHeaders(newHeaders);
                setRows(prev => {
                    const newRows = prev.map(r => {
                        const newR = { ...r };
                        delete newR[colName];
                        newR['总分'] = calculateRowTotal(newR, newHeaders);
                        return newR;
                    });
                    notifyParent(newRows, newHeaders);
                    return newRows;
                });
                setSelectedCol(null);
            }
        } else {
            alert("未找到该列");
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
        XLSX.writeFile(wb, `${fileName || "export"}.xlsx`);
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

                {/* Close Button */}
                <button
                    onClick={onClose}
                    className="absolute top-4 right-4 w-8 h-8 flex items-center justify-center text-gray-400 hover:text-gray-600 hover:bg-gray-100 rounded-lg transition-all z-20"
                >
                    ×
                </button>

                {/* Header */}
                <div className="p-6 pb-4 flex flex-col items-center justify-center gap-2 border-b border-gray-100 bg-gradient-to-r from-gray-50 to-white">
                    <div className="flex items-center gap-3">
                        <div className="w-10 h-10 rounded-xl bg-gradient-to-br from-emerald-400 to-teal-500 flex items-center justify-center shadow-lg">
                            <span className="text-white text-lg">📊</span>
                        </div>
                        <div>
                            <h2 className="text-gray-800 font-bold text-lg">{fileName}</h2>
                            <p className="text-xs text-gray-500">学生属性表 · 包含 {headers.filter(h => !['学号', '姓名', '总分'].includes(h)).length} 个评分项</p>
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
                                        onClick={() => !['学号', '姓名', '总分'].includes(h) && setSelectedCol(h === selectedCol ? null : h)}
                                        className={`border-b border-r border-gray-200 p-3 text-gray-700 min-w-[80px] font-semibold text-xs uppercase tracking-wide cursor-pointer transition-all ${selectedCol === h
                                                ? 'bg-blue-50 text-blue-600'
                                                : 'bg-gray-50 hover:bg-gray-100'
                                            } ${h === '总分' ? 'bg-emerald-50 text-emerald-700' : ''}`}
                                    >
                                        {h}
                                    </th>
                                ))}
                            </tr>
                        </thead>
                        <tbody>
                            {rows.map((row, idx) => (
                                <tr
                                    key={row._id}
                                    onClick={() => setSelectedRowId(row._id)}
                                    className={`cursor-pointer transition-colors ${selectedRowId === row._id ? 'bg-blue-50' : 'hover:bg-gray-50'}`}
                                >
                                    <td className="border-b border-r border-gray-100 bg-gray-50/50 text-center text-gray-400 text-xs font-mono select-none p-2">{idx + 1}</td>
                                    {headers.map(col => {
                                        const isScore = !['学号', '姓名', '总分'].includes(col);
                                        const val = row[col];
                                        const hasComment = row[`_comment_${col}`] ? true : false;

                                        return (
                                            <td
                                                key={col}
                                                className={`border-b border-r border-gray-100 p-2 text-center ${selectedCol === col ? 'bg-blue-50/50' : ''
                                                    } ${hasComment ? 'bg-yellow-50' : ''} ${col === '总分' ? 'bg-emerald-50/30' : ''
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
                                                ) : (
                                                    <span className={`font-medium px-2 ${col === '总分' ? 'text-emerald-600 font-bold' : 'text-gray-800'}`}>
                                                        {val}
                                                    </span>
                                                )}
                                            </td>
                                        );
                                    })}
                                </tr>
                            ))}
                        </tbody>
                    </table>
                </div>

            </div>
        </div>
    );
};
export default MidtermGradeModal;
