#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>   
#include <QPoint>
#include <QRect>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;
private slots:
    void on_toolButtonClose_clicked();
    void on_toolButtonMin_clicked();
    void on_toolButtonMax_clicked();
    void on_toolButtonRestore_clicked();

private:
    Ui::MainWindow *ui;


    // 调整区域检测
    enum ResizeRegion {
        NoResize,
        ResizeLeft,
        ResizeRight,
        ResizeTop,
        ResizeBottom,
        ResizeTopLeft,
        ResizeTopRight,
        ResizeBottomLeft,
        ResizeBottomRight
    };
    ResizeRegion detectResizeRegion(const QPoint& pos);
    void updateCursorForRegion(ResizeRegion r);
    void performResize(const QPoint& globalPos);

    // 拖动/最大化状态
    bool m_mousePressed = false;  
    bool m_leftButton = false; // 鼠标按下 
    bool m_moving = false;
    bool m_resizing = false;
    ResizeRegion m_resizeRegion = NoResize;  // 当前调整区域
    QPoint m_dragOffsetForMove; // 鼠标相对于窗口左上角的偏移量  拖动窗口时用 标题栏的坐标
    QPoint m_dragStartForResize;  // 鼠标相对于窗口左上角的偏移量 拉伸时用
    bool m_isMaximizedCustom = false;   // 自定义最大化

    const int m_borderWidth = 8; // detection width for resize
    int m_titleBarHeight = 30; // 如果你想要一个特定的上限；构造时候取一下 widgetTitleBar->height（）
    QRect m_restoreGeometry;      // 最大化前的窗口位置
    QRect m_dragStartGeometry;  // 拖动开始时的窗口位置
    QPoint m_dragStartPosGlobal;  // 鼠标开始拖动的全局位置
    // helper functions
    void setCustomMaximized(bool on);
    QRect availableScreenGeometry() const;
    void trySnapToEdgesWhenDragging(const QPoint& globalPos);
    void InitWindows();
};
#endif // MAINWINDOW_H
