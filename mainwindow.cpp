#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    InitWindows();

}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::InitWindows()
{
    // 无边框
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false); // 若需要透明/阴影可调整

    // 让 UI 中的 widgetTitleBar 捕获鼠标事件（用于双击/拖动）
    ui->widgetTitleBar->installEventFilter(this);

    // 连接按钮（确保名字与 ui 中一致）
    connect(ui->toolButtonClose, &QAbstractButton::clicked, this, &MainWindow::on_toolButtonClose_clicked);
    connect(ui->toolButtonMin, &QAbstractButton::clicked, this, &MainWindow::on_toolButtonMin_clicked);
    connect(ui->toolButtonMax, &QAbstractButton::clicked, this, &MainWindow::on_toolButtonMax_clicked);
    connect(ui->toolButtonRestore, &QAbstractButton::clicked, this, &MainWindow::on_toolButtonRestore_clicked);

    // 初始隐藏 restore 按钮（最大化时显示）
    ui->toolButtonRestore->setVisible(false);
    // 整个窗口上捕获事件
    ui->widgetTitleBar->setMouseTracking(true);   // 鼠标悬停时显示按钮
    this->setMouseTracking(true);
    ui->centralwidget->setMouseTracking(true);   // 鼠标悬停时显示按钮
    m_titleBarHeight = ui->widgetTitleBar->height();

}

void MainWindow::on_toolButtonClose_clicked()
{
    close();
}

void MainWindow::on_toolButtonMin_clicked()
{
    showMinimized();
}

void MainWindow::on_toolButtonMax_clicked()
{
    // 使用自定义最大化（保存原几何）
    if (!m_isMaximizedCustom)
    {
        m_restoreGeometry = geometry();
    }
    setCustomMaximized(true);
    ui->toolButtonMax->setVisible(false);
    ui->toolButtonRestore->setVisible(true);
}

void MainWindow::on_toolButtonRestore_clicked()
{
    setCustomMaximized(false);
    ui->toolButtonMax->setVisible(true);
    ui->toolButtonRestore->setVisible(false);
    // 还原上一次记录的窗口位置和大小
    if (!m_restoreGeometry.isEmpty())
        setGeometry(m_restoreGeometry);
}

//获取主屏幕的可用几何区域 获取当前窗口所在屏幕的可用区域
QRect MainWindow::availableScreenGeometry() const
{
    QScreen* screen = QGuiApplication::screenAt(frameGeometry().center());
    if (!screen) 
        screen = QGuiApplication::primaryScreen();
    return screen ? screen->availableGeometry() : QRect();
}

void MainWindow::setCustomMaximized(bool on)
{
    if (on == m_isMaximizedCustom) 
        return;
    if (on) {
        // 获取当前窗口所在屏幕的可用区域
        QRect avail = availableScreenGeometry();
        setGeometry(avail);
        m_isMaximizedCustom = true;
    }
    else {
        m_isMaximizedCustom = false;
        //恢复将由调用者使用m_restoreGeometry执行
        //这里只需要清除窗口状态

    }
}

// 事件过滤器：捕获widgetTitleBar上的事件（双击并按下）
bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->widgetTitleBar)   // 播放标题栏
    {
        if (event->type() == QEvent::MouseButtonDblClick) {
            // 双击标题栏
            if (m_isMaximizedCustom) {
                on_toolButtonRestore_clicked();
            }
            else {
                on_toolButtonMax_clicked();
            }
            return true;
        }   
        else if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                m_mousePressed = true;
                m_leftButton = true;
                // 记录拖动开始时的窗口位置
                m_dragPosLeftTop = me->globalPos() - frameGeometry().topLeft();

            }
            return false; // still allow other handlers
        }
        else if (event->type() == QEvent::MouseButtonRelease) {
            m_mousePressed = false;
            m_leftButton = false;
            // on release after dragging, consider snapping
            trySnapToEdgesWhenDragging(QCursor::pos());
            return false;
        }
        else if (event->type() == QEvent::MouseMove) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if (m_leftButton && m_mousePressed) {
                // if currently maximized and user drags titlebar -> restore and reposition so cursor stays at titlebar
                if (m_isMaximizedCustom) {
                    // compute new width = previous restore width (use half screen or 80% of available)
                    QRect avail = availableScreenGeometry();
                    int newW = qMin(m_restoreGeometry.width(), avail.width());
                    int newH = qMin(m_restoreGeometry.height(), avail.height());

                    // if restore geometry is empty, pick a heuristic
                    if (m_restoreGeometry.isEmpty()) {
                        newW = qMin(width(), static_cast<int>(avail.width() * 0.9));
                        newH = qMin(height(), static_cast<int>(avail.height() * 0.9));
                    }
                    else {
                        newW = m_restoreGeometry.width();
                        newH = m_restoreGeometry.height();
                    }

                    // ratio of cursor x in old width
                    double relX = double(me->pos().x()) / double(width());
                    int x = me->globalX() - int(relX * newW);
                    int y = me->globalY() - m_dragPosLeftTop.y(); // maintain Y offset roughly

                    setCustomMaximized(false);
                    setGeometry(x, y, newW, newH);
                    ui->toolButtonMax->setVisible(true);
                    ui->toolButtonRestore->setVisible(false);
                }

                // normal move
                m_moving = true;
                // move window such that top-left = mouse.global - m_dragPosLeftTop
                QPoint topleft = me->globalPos() - m_dragPosLeftTop;
                move(topleft);
            }
            return false;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = true;
        m_leftButton = true;

        // 检测大小调整区域
        m_resizeRegion = detectResizeRegion(event->pos());
        if (m_resizeRegion != NoResize) {
            m_resizing = true;
            m_dragPosLeftTop = event->globalPos();
        }
        else {
            m_resizing = false;
            //如果不是在标题栏，我们可能仍然希望允许拖动如果点击标题栏子处理在eventFilter
        }
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (m_resizing && m_leftButton) {
        performResize(event->globalPos()); // 窗口拉伸
    }
    // 如果拖动窗口，不改变光标
    else if (m_leftButton && m_moving) {
        QPoint topleft = event->globalPos() - m_dragPosLeftTop;
        move(topleft);
    }
    else if (!m_leftButton) {
        // update cursor depending on edge proximity
        ResizeRegion r = detectResizeRegion(event->pos());
        updateCursorForRegion(r);
    }
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    m_mousePressed = false;
    m_leftButton = false;
    m_moving = false;
    m_resizing = false;
    m_resizeRegion = NoResize;
    unsetCursor();
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
    // 双击在eventFilter中处理的窗口标题栏:
    if (ui->widgetTitleBar->geometry().contains(event->pos())) {
        if (m_isMaximizedCustom)
            on_toolButtonRestore_clicked();
        else
            on_toolButtonMax_clicked();
    }
    QMainWindow::mouseDoubleClickEvent(event);
}

void MainWindow::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    // reset cursor when leaving window
    if (!m_resizing) unsetCursor();
    QMainWindow::leaveEvent(event);
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event); // 先调用基类实现
    // 保存初始位置用于还原
    m_restoreGeometry = geometry();
}

// detect mouse pos relative to edges for resizing
MainWindow::ResizeRegion MainWindow::detectResizeRegion(const QPoint& pos)
{
    int x = pos.x();
    int y = pos.y();
    int w = width();
    int h = height();

    bool left = x <= m_borderWidth;
    bool right = x >= w - m_borderWidth;
    bool top = y <= m_borderWidth;
    bool bottom = y >= h - m_borderWidth;

    if (top && left) 
        return ResizeTopLeft;
    if (top && right)
        return ResizeTopRight;
    if (bottom && left) 
        return ResizeBottomLeft;
    if (bottom && right)
        return ResizeBottomRight;
    if (left)
        return ResizeLeft;
    if (right)
        return ResizeRight;
    if (top)
        return ResizeTop;
    if (bottom)
        return ResizeBottom;
    return NoResize;
}

void MainWindow::updateCursorForRegion(ResizeRegion r)
{
    switch (r) {
    case ResizeTop:
    case ResizeBottom:
        setCursor(Qt::SizeVerCursor);
        break;
    case ResizeLeft:
    case ResizeRight:
        setCursor(Qt::SizeHorCursor);
        break;
    case ResizeTopLeft:
    case ResizeBottomRight:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case ResizeTopRight:
    case ResizeBottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        break;
    default:
        unsetCursor();
        break;
    }
}

void MainWindow::performResize(const QPoint& globalPos)
{
    if (m_resizeRegion == NoResize)
        return;

    QRect geom = geometry();
    QPoint diff = globalPos - m_dragPosLeftTop;
    int dx = diff.x();
    int dy = diff.y();

    switch (m_resizeRegion) {
    case ResizeLeft: {
        int newX = geom.x() + dx;
        int newW = geom.width() - dx;
        if (newW < minimumWidth()) { newX = geom.x() + (geom.width() - minimumWidth()); newW = minimumWidth(); }
        setGeometry(newX, geom.y(), newW, geom.height());
        break;
    }
    case ResizeRight: {
        int newW = geom.width() + dx;
        if (newW < minimumWidth()) newW = minimumWidth();
        setGeometry(geom.x(), geom.y(), newW, geom.height());
        break;
    }
    case ResizeTop: {
        int newY = geom.y() + dy;
        int newH = geom.height() - dy;
        if (newH < minimumHeight()) { newY = geom.y() + (geom.height() - minimumHeight()); newH = minimumHeight(); }
        setGeometry(geom.x(), newY, geom.width(), newH);
        break;
    }
    case ResizeBottom: {
        int newH = geom.height() + dy;
        if (newH < minimumHeight()) newH = minimumHeight();
        setGeometry(geom.x(), geom.y(), geom.width(), newH);
        break;
    }
    case ResizeTopLeft: {
        int newX = geom.x() + dx;
        int newY = geom.y() + dy;
        int newW = geom.width() - dx;
        int newH = geom.height() - dy;
        if (newW < minimumWidth()) { newX = geom.x() + (geom.width() - minimumWidth()); newW = minimumWidth(); }
        if (newH < minimumHeight()) { newY = geom.y() + (geom.height() - minimumHeight()); newH = minimumHeight(); }
        setGeometry(newX, newY, newW, newH);
        break;
    }
    case ResizeTopRight: {
        int newY = geom.y() + dy;
        int newW = geom.width() + dx;
        int newH = geom.height() - dy;
        if (newW < minimumWidth()) newW = minimumWidth();
        if (newH < minimumHeight()) { newY = geom.y() + (geom.height() - minimumHeight()); newH = minimumHeight(); }
        setGeometry(geom.x(), newY, newW, newH);
        break;
    }
    case ResizeBottomLeft: {
        int newX = geom.x() + dx;
        int newW = geom.width() - dx;
        int newH = geom.height() + dy;
        if (newW < minimumWidth()) { newX = geom.x() + (geom.width() - minimumWidth()); newW = minimumWidth(); }
        if (newH < minimumHeight()) newH = minimumHeight();
        setGeometry(newX, geom.y(), newW, newH);
        break;
    }
    case ResizeBottomRight: {
        int newW = geom.width() + dx;
        int newH = geom.height() + dy;
        if (newW < minimumWidth()) newW = minimumWidth();
        if (newH < minimumHeight()) newH = minimumHeight();
        setGeometry(geom.x(), geom.y(), newW, newH);
        break;
    }
    default:
        break;
    }

    // update drag base point for smooth continuous resize
    m_dragPosLeftTop = globalPos;
}

// 当鼠标移动时，检查是否需要吸附到屏幕边缘
void MainWindow::trySnapToEdgesWhenDragging(const QPoint& globalPos)
{
    // 只有在拖动时才检查是否需要吸附(m_moving true)
    if (!m_moving)
        return;

    QScreen* screen = QGuiApplication::screenAt(globalPos);
    if (!screen) screen = QGuiApplication::primaryScreen();
    QRect avail = screen->availableGeometry();

    // 吸附到屏幕边缘
    const int threshold = 10;

    // near top -> maximize
    if (globalPos.y() <= avail.top() + threshold) {
        // maximize
        if (!m_isMaximizedCustom) {
            m_restoreGeometry = geometry(); // 仅保存一次
        }
        setCustomMaximized(true);
        ui->toolButtonMax->setVisible(false);
        ui->toolButtonRestore->setVisible(true);
        return;
    }

    // near left -> left half
    if (globalPos.x() <= avail.left() + threshold) {
        m_restoreGeometry = geometry();
        QRect leftRect(avail.left(), avail.top(), avail.width() / 2, avail.height());
        setCustomMaximized(false);
        setGeometry(leftRect);
        ui->toolButtonMax->setVisible(true);
        ui->toolButtonRestore->setVisible(false);
        return;
    }

    // near right -> right half
    if (globalPos.x() >= avail.right() - threshold) {
        m_restoreGeometry = geometry();
        QRect rightRect(avail.left() + avail.width() / 2, avail.top(), avail.width() / 2, avail.height());
        setCustomMaximized(false);
        setGeometry(rightRect);
        ui->toolButtonMax->setVisible(true);
        ui->toolButtonRestore->setVisible(false);
        return;
    }
}
