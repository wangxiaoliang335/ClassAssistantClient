import { useState, useRef } from 'react';
import { X, Upload, FileSpreadsheet, AlertCircle, CheckCircle2 } from 'lucide-react';
import * as XLSX from 'xlsx';
import { invoke } from '@tauri-apps/api/core';

interface StudentImportModalProps {
    isOpen: boolean;
    onClose: () => void;
    classId: string;
    onSuccess: () => void;
}

interface SeatData {
    row: number;
    col: number;
    name: string;
    student_id: string;
    student_name: string; // The raw label or composed label
}

const StudentImportModal = ({ isOpen, onClose, classId, onSuccess }: StudentImportModalProps) => {
    const fileInputRef = useRef<HTMLInputElement>(null);
    const [previewGrid, setPreviewGrid] = useState<string[][]>([]); // 8x11 grid for preview
    const [parsedSeats, setParsedSeats] = useState<SeatData[]>([]);
    const [loading, setLoading] = useState(false);
    const [error, setError] = useState<string | null>(null);
    const [successMsg, setSuccessMsg] = useState<string | null>(null);
    const [fileName, setFileName] = useState<string>('');

    if (!isOpen) return null;

    const handleFileSelect = (e: React.ChangeEvent<HTMLInputElement>) => {
        const files = e.target.files;
        if (files && files[0]) {
            processFile(files[0]);
        }
    };

    const processFile = async (file: File) => {
        setLoading(true);
        setError(null);
        setSuccessMsg(null);
        setFileName(file.name);

        try {
            const data = await file.arrayBuffer();
            const workbook = XLSX.read(data);
            const sheetName = workbook.SheetNames[0];
            const sheet = workbook.Sheets[sheetName];

            // Convert to array of arrays
            const rawData: any[][] = XLSX.utils.sheet_to_json(sheet, { header: 1 });

            // Parse logic mirroring Qt's ScheduleDialog::importSeatFromExcel
            const seatMap = parseSeatMap(rawData);

            setParsedSeats(seatMap);
            generatePreview(seatMap);
            setLoading(false);
        } catch (err: any) {
            console.error(err);
            setError("解析文件失败: " + (err.message || "未知错误"));
            setLoading(false);
        }
    };

    const parseSeatMap = (rawData: any[][]): SeatData[] => {
        // 1. Convert all cells to strings and trim, ensuring dense arrays
        const rows: string[][] = rawData.map(row => {
            if (!Array.isArray(row)) return [];
            const dense: string[] = [];
            // Use loop to handle sparse arrays (holes)
            for (let i = 0; i < row.length; i++) {
                dense.push(String(row[i] || '').trim());
            }
            return dense;
        });

        // 2. Find "讲台" (Podium)
        let podiumRowIdx = -1;
        // Search first 5 rows
        for (let i = 0; i < Math.min(rows.length, 5); i++) {
            if (rows[i].some(cell => cell.includes('讲台'))) {
                podiumRowIdx = i;
                break;
            }
        }

        // 3. Extract relevant rows (skip podium row itself for now, logic starts after or around it)
        // Qt logic: It processes Podium Row (Row 0 of Data) AND subsequent rows.
        // But it *removes* corridors.

        // Let's implement the Clean Data phase first.
        let dataRows: string[][] = [];

        // Helper: Check if cell is podium
        const isPodium = (s: string | undefined) => (s || '').includes('讲台');
        const isCorridor = (s: string | undefined) => (s || '').includes('走廊');

        // Qt extracts data starting from podiumRow (if found) or row 0
        const startExtractRow = podiumRowIdx >= 0 ? podiumRowIdx : 0;

        // Pass 1: Extract potentially valid data rows
        for (let r = startExtractRow; r < rows.length; r++) {
            const rowData: string[] = [];
            let hasData = false;

            // If it's the podium row, handle specially? 
            // Qt logic: "Extract all cells to left and right of podium, skipping podium itself"
            // For other rows: "Skip podium col, keep others"

            // Simplified approach: Just capture everything, then filter columns
            // Actually, we need to correctly identify columns to drop (Corridors)
            // Qt finds "Corridor" in ANY row and marks that column index for removal.

            for (let c = 0; c < rows[r].length; c++) {
                const cell = rows[r][c];
                if (!isPodium(cell)) {
                    rowData.push(cell);
                    if (cell && !isCorridor(cell)) hasData = true; // Real data
                } else {
                    // Empty placeholder for podium to keep alignment? 
                    // Qt says: "continue; // Skip podium" -> This effectively shifts left!
                    // But wait, "Extract cells... including empty ones".
                    // If we skip podium, we shift left.
                }
            }

            // Only add row if it has relevant data (or is corridor)
            if (hasData) {
                dataRows.push(rowData);
            }
        }

        // Pass 2: Identify Corridor columns
        const corridorColIndices = new Set<number>();
        for (let r = 0; r < dataRows.length; r++) {
            for (let c = 0; c < dataRows[r].length; c++) {
                if (isCorridor(dataRows[r][c])) {
                    corridorColIndices.add(c);
                }
            }
        }

        // Pass 3: Remove Corridor columns (Right to Left to extract cleanly)
        const sortedCorridorCols = Array.from(corridorColIndices).sort((a, b) => b - a);
        for (let c of sortedCorridorCols) {
            for (let r = 0; r < dataRows.length; r++) {
                if (c < dataRows[r].length) {
                    dataRows[r].splice(c, 1);
                }
            }
        }

        // Pass 4: Remove leading empty columns
        // Qt: "Remove all rows' first column if ALL rows' first column is empty"
        // let removedAny = false;
        while (true) {
            if (dataRows.length === 0) break;
            let firstColEmpty = true;
            for (let r = 0; r < dataRows.length; r++) {
                if (dataRows[r].length > 0 && dataRows[r][0] !== '') {
                    firstColEmpty = false;
                    break;
                }
            }

            if (firstColEmpty) {
                for (let r = 0; r < dataRows.length; r++) {
                    if (dataRows[r].length > 0) dataRows[r].shift();
                }
                // removedAny = true;
            } else {
                break;
            }
        }

        // 4. Map to Seat Grid (fillSeatTableFromData)
        // Seat Grid is 8 rows (0-7), 11 cols (0-10)
        const seats: SeatData[] = [];
        const maxImportRows = Math.min(dataRows.length, 8);

        for (let r = 0; r < maxImportRows; r++) {
            const actualRowData = dataRows[r];
            let dataIndex = 0; // Consumption pointer for actualRowData

            if (r === 0) {
                // Row 0 Layout: Cols 0, 1, 3, 7, 9, 10
                // Qt logic: iterates these specific columns and fills from actualRowData sequentially.
                // It skips empty data cells but advances seat pointer? 
                // Wait, Qt logic: 
                // seatCols = {0, 1, 3, 7, 9, 10}; 
                // loops i in actualRowData:
                //   cellText = actualRowData[i]
                //   if empty: seatIndex++ (skip seat position), continue
                //   if valid: fill seatCols[seatIndex], seatIndex++

                const seatCols = [0, 1, 3, 7, 9, 10];
                let seatIndex = 0;

                for (let i = 0; i < actualRowData.length; i++) {
                    if (seatIndex >= seatCols.length) break;

                    const cellText = actualRowData[i];
                    if (!cellText) {
                        seatIndex++; // Skip this seat slot
                        continue;
                    }

                    // Parse info
                    const info = parseStudentInfo(cellText);
                    const seatCol = seatCols[seatIndex];

                    if (info.name || info.id) {
                        seats.push({
                            row: r + 1, // 1-based
                            col: seatCol + 1, // 1-based
                            name: info.name,
                            student_id: info.id || '',
                            student_name: cellText
                        });
                    }
                    seatIndex++;
                }

            } else {
                // Row > 0 Layout: 4 Blocks
                // Block 1: Cols 0-1
                // Block 2: Cols 3-4 (Skip 2)
                // Block 3: Cols 6-7 (Skip 5)
                // Block 4: Cols 9-10 (Skip 8)

                // Block 1
                for (let c = 0; c < 2 && dataIndex < actualRowData.length; c++) {
                    const cell = actualRowData[dataIndex++];
                    if (cell) {
                        const info = parseStudentInfo(cell);
                        seats.push({ row: r + 1, col: c + 1, name: info.name, student_id: info.id || '', student_name: cell });
                    }
                }

                // Block 2 (Cols 3, 4)
                for (let c = 3; c < 5 && dataIndex < actualRowData.length; c++) {
                    const cell = actualRowData[dataIndex++];
                    if (cell) {
                        const info = parseStudentInfo(cell);
                        seats.push({ row: r + 1, col: c + 1, name: info.name, student_id: info.id || '', student_name: cell });
                    }
                }

                // Block 3 (Cols 6, 7)
                for (let c = 6; c < 8 && dataIndex < actualRowData.length; c++) {
                    const cell = actualRowData[dataIndex++];
                    if (cell) {
                        const info = parseStudentInfo(cell);
                        seats.push({ row: r + 1, col: c + 1, name: info.name, student_id: info.id || '', student_name: cell });
                    }
                }

                // Block 4 (Cols 9, 10)
                for (let c = 9; c < 11 && dataIndex < actualRowData.length; c++) {
                    const cell = actualRowData[dataIndex++];
                    if (cell) {
                        const info = parseStudentInfo(cell);
                        seats.push({ row: r + 1, col: c + 1, name: info.name, student_id: info.id || '', student_name: cell });
                    }
                }
            }
        }

        return seats;
    };

    const parseStudentInfo = (text: string): { name: string, id?: string } => {
        // Regex from Qt
        // 1. Slash: "Name/ID" or "NameID/ID"
        let s = text.trim();
        let idFromSlash = '';
        const slashIdx = s.lastIndexOf('/');
        if (slashIdx >= 0) {
            idFromSlash = s.substring(slashIdx + 1).trim();
            s = s.substring(0, slashIdx).trim();
        }

        // 2. Name: Chinese chars at start
        let name = '';
        const nameMatch = s.match(/^([\u4e00-\u9fa5]+)/);
        if (nameMatch) {
            name = nameMatch[1];
        }

        // 3. ID: Digits-Digits
        let id = idFromSlash;
        if (!id) {
            const dashMatch = s.match(/(\d+-\d+)/);
            if (dashMatch) {
                id = dashMatch[1];
            } else if (!name) {
                // Fallback split
                const parts = s.split(/[\s-]+/);
                if (parts.length > 0) name = parts[0];
                if (parts.length >= 2) id = parts[parts.length - 1];
            }
        }

        if (!name) name = text.trim();

        return { name, id };
    };

    const generatePreview = (seats: SeatData[]) => {
        // Create 8x11 empty grid
        const grid: string[][] = Array(8).fill(null).map(() => Array(11).fill(''));
        seats.forEach(seat => {
            if (seat.row >= 1 && seat.row <= 8 && seat.col >= 1 && seat.col <= 11) {
                // Display: Name
                grid[seat.row - 1][seat.col - 1] = seat.name;
            }
        });
        setPreviewGrid(grid);
    };

    const handleConfirm = async () => {
        if (!classId || parsedSeats.length === 0) return;
        setLoading(true);
        try {
            await invoke('save_seat_arrangement', {
                classId: classId,
                seatsJson: JSON.stringify({
                    class_id: classId,
                    seats: parsedSeats
                })
            });
            setSuccessMsg("导入成功！已刷新座位表。");
            setTimeout(() => {
                onSuccess();
                onClose();
            }, 1000);
        } catch (err: any) {
            console.error(err);
            setError("上传失败: " + (err.message || "服务器错误"));
            setLoading(false);
        }
    };

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/50 backdrop-blur-sm">
            <div className="w-[900px] h-[700px] bg-white dark:bg-[#1a1b1e] rounded-xl shadow-2xl flex flex-col overflow-hidden animate-in fade-in zoom-in duration-200">

                {/* Header */}
                <div className="flex items-center justify-between px-6 py-4 border-b border-gray-200 dark:border-gray-800">
                    <div className="flex items-center gap-2">
                        <div className="bg-blue-100 dark:bg-blue-900/30 p-2 rounded-lg">
                            <FileSpreadsheet className="w-5 h-5 text-blue-600 dark:text-blue-400" />
                        </div>
                        <h2 className="text-xl font-semibold text-gray-800 dark:text-gray-100">
                            导入座位表布局
                        </h2>
                    </div>
                    <button
                        onClick={onClose}
                        className="p-2 hover:bg-gray-100 dark:hover:bg-gray-800 rounded-full transition-colors"
                    >
                        <X className="w-5 h-5 text-gray-500" />
                    </button>
                </div>

                {/* Content */}
                <div className="flex-1 p-6 flex flex-col gap-6 overflow-hidden">

                    {/* File Upload Area */}
                    <div className="flex-none">
                        <div
                            onClick={() => fileInputRef.current?.click()}
                            className="border-2 border-dashed border-gray-300 dark:border-gray-700 rounded-xl p-8 flex flex-col items-center justify-center gap-3 cursor-pointer hover:border-blue-500 dark:hover:border-blue-400 hover:bg-blue-50 dark:hover:bg-blue-900/10 transition-all group"
                        >
                            <input
                                type="file"
                                ref={fileInputRef}
                                onChange={handleFileSelect}
                                accept=".xlsx,.xls"
                                className="hidden"
                            />
                            <div className="bg-blue-100 dark:bg-blue-900/50 p-4 rounded-full group-hover:scale-110 transition-transform">
                                <Upload className="w-8 h-8 text-blue-600 dark:text-blue-400" />
                            </div>
                            <div className="text-center">
                                <p className="text-sm font-medium text-gray-700 dark:text-gray-300">
                                    {fileName ? fileName : "点击选择座位表 Excel 文件"}
                                </p>
                                <p className="text-xs text-gray-500 mt-1">
                                    支持 .xlsx, .xls 格式，需包含“讲台”标识
                                </p>
                            </div>
                        </div>
                    </div>

                    {/* Preview Grid */}
                    {previewGrid.length > 0 && (
                        <div className="flex-1 flex flex-col min-h-0">
                            <h3 className="text-sm font-medium text-gray-700 dark:text-gray-300 mb-2 flex items-center gap-2">
                                布局预览 <span className="text-xs font-normal text-gray-500">({parsedSeats.length} 个座位的学生信息)</span>
                            </h3>
                            <div className="flex-1 overflow-auto border border-gray-200 dark:border-gray-800 rounded-lg p-4 bg-gray-50 dark:bg-[#151618]">
                                <div className="grid gap-2" style={{ gridTemplateColumns: 'repeat(11, minmax(60px, 1fr))' }}>
                                    {/* Column Headers (1-11) */}
                                    {Array.from({ length: 11 }).map((_, i) => (
                                        <div key={`h-${i}`} className="text-center text-xs text-gray-400 font-mono mb-1">
                                            {i + 1}
                                        </div>
                                    ))}

                                    {/* Grid Cells */}
                                    {previewGrid.map((row, rIdx) => (
                                        row.map((cell, cIdx) => (
                                            <div
                                                key={`${rIdx}-${cIdx}`}
                                                className={`
                                                    h-10 flex items-center justify-center text-xs rounded border transition-colors
                                                    ${cell
                                                        ? 'bg-white dark:bg-[#25262b] border-blue-200 dark:border-blue-800 text-gray-800 dark:text-gray-200 shadow-sm'
                                                        : 'border-transparent text-gray-300 dark:text-gray-700'
                                                    }
                                                `}
                                            >
                                                <span className="truncate px-1" title={cell}>{cell || '·'}</span>
                                            </div>
                                        ))
                                    ))}
                                </div>
                            </div>
                        </div>
                    )}

                    {/* Messages */}
                    {error && (
                        <div className="flex items-center gap-2 text-red-600 bg-red-50 dark:bg-red-900/20 p-3 rounded-lg text-sm animate-in slide-in-from-top-2">
                            <AlertCircle className="w-4 h-4" />
                            {error}
                        </div>
                    )}
                    {successMsg && (
                        <div className="flex items-center gap-2 text-green-600 bg-green-50 dark:bg-green-900/20 p-3 rounded-lg text-sm animate-in slide-in-from-top-2">
                            <CheckCircle2 className="w-4 h-4" />
                            {successMsg}
                        </div>
                    )}
                </div>

                {/* Footer */}
                <div className="p-6 pt-0 flex justify-end gap-3">
                    <button
                        onClick={onClose}
                        className="px-4 py-2 text-sm font-medium text-gray-700 dark:text-gray-300 hover:bg-gray-100 dark:hover:bg-gray-800 rounded-lg transition-colors"
                    >
                        取消
                    </button>
                    <button
                        onClick={handleConfirm}
                        disabled={loading || parsedSeats.length === 0}
                        className="px-4 py-2 text-sm font-medium text-white bg-blue-600 hover:bg-blue-700 disabled:opacity-50 disabled:cursor-not-allowed rounded-lg shadow-sm transition-colors flex items-center gap-2"
                    >
                        {loading && <div className="w-4 h-4 border-2 border-white/30 border-t-white rounded-full animate-spin" />}
                        确认导入
                    </button>
                </div>
            </div>
        </div>
    );
};

export default StudentImportModal;
