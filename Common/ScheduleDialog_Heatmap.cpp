#include "ScheduleDialog.h"
#include "HeatmapSegmentDialog.h"
#include "HeatmapViewDialog.h"
#include "HeatmapTypes.h"
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QTextStream>

// 实现热力图相关方法
void ScheduleDialog::showSegmentDialog()
{
	if (!heatmapSegmentDlg) {
		heatmapSegmentDlg = new HeatmapSegmentDialog(this);
		heatmapSegmentDlg->setStudentData(m_students);
		QStringList tables, attributes;
		buildHeatmapOptions(tables, attributes);
		if (!tables.isEmpty()) heatmapSegmentDlg->setTableOptions(tables);
		if (!attributes.isEmpty()) heatmapSegmentDlg->setAttributeOptions(attributes);
		connect(heatmapSegmentDlg, &QDialog::accepted, this, [this]() {
			if (heatmapSegmentDlg) {
				m_segments = heatmapSegmentDlg->getSegments();
				m_selectedAttribute = heatmapSegmentDlg->selectedAttribute(); // 保存选定的属性名称
				m_selectedTable = heatmapSegmentDlg->selectedTable(); // 保存选定的表格名称（Excel文件名）
				m_heatmapType = 1; // 分段图
				updateSeatColors();
			}
		});
	}
	else {
		// 每次打开前刷新下拉选项
		QStringList tables, attributes;
		buildHeatmapOptions(tables, attributes);
		if (!tables.isEmpty()) heatmapSegmentDlg->setTableOptions(tables);
		if (!attributes.isEmpty()) heatmapSegmentDlg->setAttributeOptions(attributes);
	}
	heatmapSegmentDlg->show();
	heatmapSegmentDlg->raise();
	heatmapSegmentDlg->activateWindow();
}

void ScheduleDialog::showGradientHeatmap()
{
	if (!heatmapViewDlg) {
		heatmapViewDlg = new HeatmapViewDialog(this);
		heatmapViewDlg->setStudentData(m_students);
		heatmapViewDlg->setHeatmapType(2); // 渐变热力图
		// 提供属性获取器，便于按表更新属性列表
		heatmapViewDlg->setAttributeFetcher([this](const QString& tableName) {
			return getAttributesForTable(tableName);
		});
	}
	// 每次打开前刷新表格/属性下拉与学生数据
	QStringList tables, attributes;
	buildHeatmapOptions(tables, attributes);
	heatmapViewDlg->setTableOptions(tables);
	// 默认表选择：已选表，或者第一个
	QString currentTable = m_selectedTable.isEmpty() ? (tables.isEmpty() ? QString() : tables.first()) : m_selectedTable;
	if (!currentTable.isEmpty()) {
		heatmapViewDlg->setSelectedTable(currentTable);
	}
	// 属性列表随表格刷新；若有已选属性则选中
	if (!m_selectedAttribute.isEmpty()) {
		heatmapViewDlg->setSelectedAttribute(m_selectedAttribute);
	}
	heatmapViewDlg->setStudentData(m_students);
	
	// 收集座位映射信息（学生ID -> (行, 列)）
	if (seatTable) {
		QMap<QString, QPair<int, int>> seatMap;
		int maxRows = seatTable->rowCount();
		int maxCols = seatTable->columnCount();
		
		for (int row = 0; row < maxRows; ++row) {
			for (int col = 0; col < maxCols; ++col) {
				QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
				if (btn && btn->property("isSeat").toBool()) {
					QString studentId = btn->property("studentId").toString();
					if (!studentId.isEmpty()) {
						seatMap[studentId] = QPair<int, int>(row, col);
					}
				}
			}
		}
		
		// 设置座位映射
		heatmapViewDlg->setSeatMap(seatMap, maxRows, maxCols);
	}
	
	heatmapViewDlg->show();
	heatmapViewDlg->raise();
	heatmapViewDlg->activateWindow();
}

void ScheduleDialog::setSegments(const QList<struct SegmentRange>& segments)
{
	m_segments = segments;
	m_heatmapType = 1;
	updateSeatColors();
}

// 构建热力图下拉选项（表、科目），来源于 excel_files/<schoolId>/<classId> 下的表头
// 现在支持扫描 group/ 和 student/ 两个子目录
void ScheduleDialog::buildHeatmapOptions(QStringList& tables, QStringList& attributes)
{
	tables.clear();
	attributes.clear();

	UserInfo userInfo = CommonInfo::GetData();
	QString schoolId = userInfo.schoolId;
	if (schoolId.isEmpty() || m_classid.isEmpty()) {
		return;
	}

	QString baseDir = QCoreApplication::applicationDirPath() + "/excel_files";
	QString classDir = baseDir + "/" + schoolId + "/" + m_classid;

	QStringList filters;
	filters << "*.xlsx" << "*.xls" << "*.csv";
	QFileInfoList fileList;
	
	// 扫描主目录（向后兼容）
	QDir dir(classDir);
	if (dir.exists()) {
		QFileInfoList mainFiles = dir.entryInfoList(filters, QDir::Files);
		fileList.append(mainFiles);
	}
	
	// 扫描 group/ 子目录
	QDir groupDir(classDir + "/group");
	if (groupDir.exists()) {
		QFileInfoList groupFiles = groupDir.entryInfoList(filters, QDir::Files);
		fileList.append(groupFiles);
	}
	
	// 扫描 student/ 子目录
	QDir studentDir(classDir + "/student");
	if (studentDir.exists()) {
		QFileInfoList studentFiles = studentDir.entryInfoList(filters, QDir::Files);
		fileList.append(studentFiles);
	}
	
	// 只获取表格列表，不获取属性
	for (const QFileInfo& fi : fileList) {
		QString fileName = fi.baseName();
		if (!tables.contains(fileName)) {
			tables << fileName;
		}
	}
	
	// 如果有表格，获取第一个表格的属性作为初始值
	if (!tables.isEmpty() && !fileList.isEmpty()) {
		QStringList headers = getHeadersFromFile(fileList.first());
		for (const QString& h : headers) {
			QString trimmed = h.trimmed();
			if (trimmed.isEmpty()) continue;
			if (trimmed == "学号" || trimmed == "姓名" || trimmed == "小组" || trimmed == "组号") continue;
			if (!attributes.contains(trimmed)) {
				attributes << trimmed;
			}
		}
	}
}

// 根据表格名称获取该表格的属性列表
// 现在支持在 group/ 和 student/ 两个子目录中查找
QStringList ScheduleDialog::getAttributesForTable(const QString& tableName)
{
	QStringList attributes;
	
	UserInfo userInfo = CommonInfo::GetData();
	QString schoolId = userInfo.schoolId;
	if (schoolId.isEmpty() || m_classid.isEmpty() || tableName.isEmpty()) {
		return attributes;
	}

	QString baseDir = QCoreApplication::applicationDirPath() + "/excel_files";
	QString classDir = baseDir + "/" + schoolId + "/" + m_classid;

	QStringList filters;
	filters << "*.xlsx" << "*.xls" << "*.csv";
	
	// 按优先级查找：先查找主目录，再查找 group/，最后查找 student/
	QStringList searchDirs;
	searchDirs << classDir << (classDir + "/group") << (classDir + "/student");
	
	for (const QString& searchDir : searchDirs) {
		QDir dir(searchDir);
		if (!dir.exists()) continue;
		
		QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);
		
		// 查找匹配的表格文件
		for (const QFileInfo& fi : fileList) {
			if (fi.baseName() == tableName) {
				QStringList headers = getHeadersFromFile(fi);
				for (const QString& h : headers) {
					QString trimmed = h.trimmed();
					if (trimmed.isEmpty()) continue;
					if (trimmed == "学号" || trimmed == "姓名" || trimmed == "小组" || trimmed == "组号") continue;
					if (!attributes.contains(trimmed)) {
						attributes << trimmed;
					}
				}
				return attributes; // 找到第一个匹配的就返回
			}
		}
	}
	
	return attributes;
}

// 从文件中读取表头（辅助函数）
QStringList ScheduleDialog::getHeadersFromFile(const QFileInfo& fi)
{
	QStringList headers;
	QString suffix = fi.suffix().toLower();
	
	if (suffix == "xlsx" || suffix == "xls") {
		QXlsx::Document xlsx(fi.absoluteFilePath());
		int col = 1;
		while (true) {
			QString val = xlsx.read(1, col).toString();
			if (val.isEmpty()) break;
			headers << val;
			++col;
		}
	} else if (suffix == "csv") {
		QFile f(fi.absoluteFilePath());
		if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QTextStream ts(&f);
			ts.setCodec("UTF-8");
			if (!ts.atEnd()) {
				QString line = ts.readLine();
				headers = line.split(',', Qt::KeepEmptyParts);
			}
			f.close();
		}
	}
	
	return headers;
}

void ScheduleDialog::updateSeatColors()
{
	if (!seatTable || m_students.isEmpty()) return;
	
	// 获取所有座位按钮
	QList<QPushButton*> seatButtons;
	for (int row = 0; row < seatTable->rowCount(); ++row) {
		for (int col = 0; col < seatTable->columnCount(); ++col) {
			QPushButton* btn = qobject_cast<QPushButton*>(seatTable->cellWidget(row, col));
			if (btn && btn->property("isSeat").toBool()) {
				seatButtons.append(btn);
			}
		}
	}
	
	if (m_heatmapType == 1) {
		// 分段图：根据分段区间设置颜色
		// 使用选定的属性值来设置颜色
		if (m_selectedAttribute.isEmpty()) {
			// 如果没有选定属性，不设置颜色
			return;
		}
		
		for (QPushButton* btn : seatButtons) {
			QString studentId = btn->property("studentId").toString();
			if (studentId.isEmpty()) continue;
			
			// 查找对应的学生
			StudentInfo* student = nullptr;
			for (auto& s : m_students) {
				if (s.id == studentId) {
					student = &s;
					break;
				}
			}
			
			if (!student) continue;
			
			// 从 attributes 中获取对应属性的值（支持按Excel文件选择）
			double attributeValue = 0.0;
			bool hasValue = false;
			
			// 优先从 attributesByExcel 中获取（如果指定了Excel文件名）
			if (!m_selectedTable.isEmpty() && student->attributesByExcel.contains(m_selectedTable)) {
				const QMap<QString, double>& excelAttrs = student->attributesByExcel[m_selectedTable];
				if (excelAttrs.contains(m_selectedAttribute)) {
					attributeValue = excelAttrs[m_selectedAttribute];
					hasValue = true;
				}
			}
			
			// 如果没有找到，尝试从 attributesFull 中获取（复合键名）
			if (!hasValue && !m_selectedTable.isEmpty()) {
				QString compositeKey = QString("%1_%2").arg(m_selectedAttribute).arg(m_selectedTable);
				if (student->attributesFull.contains(compositeKey)) {
					attributeValue = student->attributesFull[compositeKey];
					hasValue = true;
				}
			}
			
			// 向后兼容：如果还没有找到，从 attributes 中获取（第一个值）
			if (!hasValue && student->attributes.contains(m_selectedAttribute)) {
				attributeValue = student->attributes[m_selectedAttribute];
				hasValue = true;
			}
			
			if (!hasValue) {
				// 如果学生没有该属性，跳过
				continue;
			}
			
			// 根据属性值找到对应的分段
			QColor seatColor = QColor("#dc3545"); // 默认红色
			for (const auto& segment : m_segments) {
				if (attributeValue >= segment.minValue && attributeValue <= segment.maxValue) {
					seatColor = segment.color;
					break;
				}
			}
			
			// 设置按钮背景色
			btn->setStyleSheet(QString(
				"QPushButton { "
				"background-color: %1; "
				"color: white; "
				"border: 1px solid #ccc; "
				"border-radius: 4px; "
				"padding: 5px; "
				"font-size: 12px; "
				"}"
				"QPushButton:hover { "
				"background-color: %2; "
				"}"
			).arg(seatColor.name()).arg(seatColor.darker(120).name()));
		}
	} else if (m_heatmapType == 2) {
		// 渐变图：根据成绩计算渐变颜色
		// 找到最小和最大成绩
		double minScore = m_students[0].score;
		double maxScore = m_students[0].score;
		for (const auto& student : m_students) {
			if (student.score < minScore) minScore = student.score;
			if (student.score > maxScore) maxScore = student.score;
		}
		
		double range = maxScore - minScore;
		if (range == 0) range = 1; // 避免除零
		
		for (QPushButton* btn : seatButtons) {
			QString studentId = btn->property("studentId").toString();
			if (studentId.isEmpty()) continue;
			
			// 查找对应的学生
			StudentInfo* student = nullptr;
			for (auto& s : m_students) {
				if (s.id == studentId) {
					student = &s;
					break;
				}
			}
			
			if (!student) continue;
			
			// 计算归一化值 (0-1)
			double normalized = (student->score - minScore) / range;
			
			// 计算渐变颜色：蓝色(低) -> 绿色 -> 黄色 -> 红色(高)
			QColor seatColor;
			if (normalized < 0.33) {
				// 蓝色到绿色
				double t = normalized / 0.33;
				seatColor = QColor(
					int(0 + t * 0),
					int(0 + t * 255),
					int(255 - t * 0)
				);
			} else if (normalized < 0.66) {
				// 绿色到黄色
				double t = (normalized - 0.33) / 0.33;
				seatColor = QColor(
					int(0 + t * 255),
					255,
					int(255 - t * 255)
				);
			} else {
				// 黄色到红色
				double t = (normalized - 0.66) / 0.34;
				seatColor = QColor(
					255,
					int(255 - t * 255),
					0
				);
			}
			
			// 设置按钮背景色
			btn->setStyleSheet(QString(
				"QPushButton { "
				"background-color: %1; "
				"color: white; "
				"border: 1px solid #ccc; "
				"border-radius: 4px; "
				"padding: 5px; "
				"font-size: 12px; "
				"}"
				"QPushButton:hover { "
				"background-color: %2; "
				"}"
			).arg(seatColor.name()).arg(seatColor.darker(120).name()));
		}
	}
}

// 刷新热力图选项（如果需要）
void ScheduleDialog::refreshHeatmapOptionsIfNeeded()
{
	// 如果分段对话框已打开，刷新其选项
	if (heatmapSegmentDlg && !heatmapSegmentDlg->isHidden()) {
		QStringList tables, attributes;
		buildHeatmapOptions(tables, attributes);
		if (!tables.isEmpty()) heatmapSegmentDlg->setTableOptions(tables);
		if (!attributes.isEmpty()) heatmapSegmentDlg->setAttributeOptions(attributes);
	}
}

