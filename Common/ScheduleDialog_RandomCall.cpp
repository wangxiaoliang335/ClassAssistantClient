#include "ScheduleDialog.h"
#include "RandomCallDialog.h"
#include <QMessageBox>

// 实现随机点名按钮的连接逻辑
void ScheduleDialog::connectRandomCallButton(QPushButton* btnRandom, QTableWidget* seatTable)
{
    if (!btnRandom) return;
    
    // 使用成员变量 this->seatTable 而不是参数 seatTable，确保指针始终有效
    connect(btnRandom, &QPushButton::clicked, this, [this]() {
        // 检查是否有学生数据
        if (m_students.isEmpty()) {
            QMessageBox::information(this, "提示", "请先上传期中成绩表并排好座位！");
            return;
        }
        
        // 使用成员变量 this->seatTable，确保指针有效
        if (!this->seatTable) {
            qWarning() << "座位表指针为空！";
            return;
        }
        
        if (!randomCallDlg) {
            randomCallDlg = new RandomCallDialog(this);
            // 优先从Excel文件加载数据
            randomCallDlg->loadExcelFiles(m_classid);
            // 如果没有Excel文件，使用现有的学生数据
            if (randomCallDlg->getParticipants().isEmpty() && !m_students.isEmpty()) {
                randomCallDlg->setStudentData(m_students);
            }
            randomCallDlg->setSeatTable(this->seatTable);
            // 连接学生成绩更新信号
            connect(randomCallDlg, &RandomCallDialog::studentScoreUpdated, this, [this](const QString& studentId, double newScore) {
                // 更新本地学生数据
                for (auto& student : m_students) {
                    if (student.id == studentId) {
                        student.score = newScore;
                        break;
                    }
                }
                // 如果热力图已设置，需要更新座位颜色
                if (m_heatmapType == 1 || m_heatmapType == 2) {
                    updateSeatColors();
                }
            });
        } else {
            // 优先从Excel文件加载数据
            randomCallDlg->loadExcelFiles(m_classid);
            // 如果没有Excel文件，使用现有的学生数据
            if (randomCallDlg->getParticipants().isEmpty() && !m_students.isEmpty()) {
                randomCallDlg->setStudentData(m_students);
            }
            randomCallDlg->setSeatTable(this->seatTable);
        }
        randomCallDlg->show();
        randomCallDlg->raise();
        randomCallDlg->activateWindow();
    });
}

