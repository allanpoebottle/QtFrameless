#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QShortcut>
#include <QGraphicsDropShadowEffect>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initWindows();

}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::initWindows()
{
    // 无边框
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, true); // 必须 true 才能显示阴影
    // 设置背景色 为白色（否则透明） 
    /*
    * 注意：setStyleSheet() 会完全替换之前的样式，所以直接调用会覆盖掉之前的背景、颜色等设置
    * 实际项目建议推荐使用 dynamicProperty + QSS 匹配结合的方式，这样不会覆盖之前的设置
    */
    ui->centralwidget->setStyleSheet("background:white;");
    // 设置边距以显示阴影
    setContentsMargins(m_margin, m_margin, m_margin, m_margin);
    // 阴影效果
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);           // 模糊半径
    shadow->setOffset(0, 0);             // 阴影偏移
    shadow->setColor(QColor(0, 0, 0, 160)); // 阴影颜色
    ui->centralwidget->setGraphicsEffect(shadow);

    // 让 UI 中的 widgetTitleBar 捕获鼠标事件（用于双击/拖动）
    m_widgetTitleBar = ui->widgetTitleBar;
    m_widgetTitleBar->installEventFilter(this);
    m_titleBarHeight = m_widgetTitleBar->height();
    // 连接按钮（确保名字与 ui 中一致）
    connect(ui->toolButtonClose, &QAbstractButton::clicked, this, &MainWindow::on_toolButtonClose_clicked);
    connect(ui->toolButtonMin, &QAbstractButton::clicked, this, &MainWindow::on_toolButtonMin_clicked);
    connect(ui->toolButtonMax, &QAbstractButton::clicked, this, &MainWindow::on_toolButtonMax_clicked);
    connect(ui->toolButtonRestore, &QAbstractButton::clicked, this, &MainWindow::on_toolButtonRestore_clicked);

    // 初始隐藏 restore 按钮（最大化时显示）
    ui->toolButtonRestore->setVisible(false);
    // 整个窗口上捕获事件
    m_widgetTitleBar->setMouseTracking(true);   // 鼠标悬停时显示按钮
    this->setMouseTracking(true);
    ui->centralwidget->setMouseTracking(true);   // 鼠标悬停时显示按钮
    // 快捷键  F11 全屏切换
    QShortcut* shortcutFullScreen = new QShortcut(QKeySequence(Qt::Key_F11), this);
    connect(shortcutFullScreen, &QShortcut::activated, this, [this]() {
        if (m_isMaximizedCustom) {
            on_toolButtonRestore_clicked();  // 已经最大化则还原
        }
        else {
            on_toolButtonMax_clicked();      // 否则最大化
        }
        });

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
        // 最大化时去掉阴影和圆角
        setContentsMargins(0, 0, 0, 0);
        if (ui->centralwidget->graphicsEffect())
            ui->centralwidget->graphicsEffect()->setEnabled(false);  // 隐藏阴影
    }
    else {
        m_isMaximizedCustom = false;
        setContentsMargins(m_margin, m_margin, m_margin, m_margin);
              if (ui->centralwidget->graphicsEffect())
                  ui->centralwidget->graphicsEffect()->setEnabled(true);
        //恢复将由调用者使用m_restoreGeometry执行
        //这里只需要清除窗口状态

    }
}

// 事件过滤器：捕获widgetTitleBar上的事件（双击并按下）
bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_widgetTitleBar)   // 播放标题栏
    {
        if (m_resizing) {
            // 如果当前正在缩放，则禁止标题栏的拖动逻辑
            return false;
        }
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
                if (!m_isMaximizedCustom)
                    m_restoreGeometry = geometry();
                // 记录鼠标拖动开始时的窗口位置
                m_dragOffsetForMove = me->globalPos() - frameGeometry().topLeft();

            }
            return false; // 仍然允许其他处理程序
        }
        else if (event->type() == QEvent::MouseButtonRelease) {
            m_mousePressed = false;
            m_leftButton = false;
            // 当鼠标移动时，检查是否需要吸附到屏幕边缘
            trySnapToEdgesWhenDragging(QCursor::pos());
            return false;
        }
        else if (event->type() == QEvent::MouseMove) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if (m_leftButton && m_mousePressed) {
                // 如果当前最大化和用户拖动标题栏->恢复和重新定位，使光标停留在标题栏
                if (m_isMaximizedCustom) {
                    // 获取 restore 矩形（最大化前窗口）
                    int restoreW = m_restoreGeometry.width();
                    int restoreH = m_restoreGeometry.height();
                    if (restoreW <= 0 || restoreH <= 0) {
                        QRect avail = availableScreenGeometry();
                        restoreW = qMin(width(), static_cast<int>(avail.width() * 0.8));
                        restoreH = qMin(height(), static_cast<int>(avail.height() * 0.8));
                    }
                    // 鼠标在当前窗口中的比例位置
                    double relX = double(me->pos().x()) / double(width());
                    double relY = double(me->pos().y()) / double(height());
                    // 计算还原窗口左上角，使鼠标保持在相同相对位置
                    int x = me->globalX() - int(relX * restoreW);
                    int y = me->globalY() - int(relY * restoreH);
                    // 计算还原后窗口左上角，使鼠标仍保持在标题栏的相对位置
                    int newX = me->globalX() - int(relX * restoreW);
                    int newY = me->globalY() - int(relY * restoreH) + 10; //向下偏移一点
                    // 边界保护：防止出屏幕
                    QRect avail = availableScreenGeometry();
                    if (newX < avail.left()) newX = avail.left();
                    if (newY < avail.top() + 1) newY = avail.top() + 1;
                    if (newX + restoreW > avail.right()) newX = avail.right() - restoreW;
                    if (newY + restoreH > avail.bottom()) newY = avail.bottom() - restoreH;

            
                    setCustomMaximized(false);
                    setGeometry(newX, newY, restoreW, restoreH);
                 
                    ui->toolButtonMax->setVisible(true);
                    ui->toolButtonRestore->setVisible(false);

                    // 更新拖动偏移，保证后续移动平滑
                    m_dragOffsetForMove = me->globalPos() - frameGeometry().topLeft();
                }
                // 正常移动窗口
                m_moving = true;
                QPoint topleft = me->globalPos() - m_dragOffsetForMove;
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
            m_dragStartForResize = event->globalPos();
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
    else if (!m_leftButton) {
        // 鼠标移动到窗口边缘更新光标
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
    if (m_widgetTitleBar->geometry().contains(event->pos())) {
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
    // 离开窗口时重置光标
    if (!m_resizing)
        unsetCursor();
    QMainWindow::leaveEvent(event);
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event); // 先调用基类实现
    // 只在第一次显示时保存初始位置  每次最小化恢复会调用showEvent
    static bool firstShow = true;
    if (firstShow) {
        firstShow = false;
        m_restoreGeometry = geometry();
    }
}

// 检测相对于边缘的鼠标位置来调整大小
MainWindow::ResizeRegion MainWindow::detectResizeRegion(const QPoint& pos)
{
    //最大化状态下不允许调整大小
    if (m_isMaximizedCustom || isMaximized())
        return NoResize;
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
    QPoint diff = globalPos - m_dragStartForResize;
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

    // 更新拖拽基点以平滑连续调整大小
    m_dragStartForResize = globalPos;
}

// 当鼠标移动时，检查是否需要吸附到屏幕边缘
void MainWindow::trySnapToEdgesWhenDragging(const QPoint& globalPos)
{
    // 只有在拖动时才检查是否需要吸附(m_moving true)
    if (!m_moving)
        return;
    // 获取鼠标位置所在屏幕的可用区域
    QScreen* screen = QGuiApplication::screenAt(globalPos);
    if (!screen) screen = QGuiApplication::primaryScreen();
    QRect avail = screen->availableGeometry();

    // 吸附到屏幕边缘
    const int threshold = 10;

    // near top -> maximize
    if (globalPos.y() <= avail.top() + threshold) {
        // 最大化 最大化的时候已经在首次点击时，保存当前窗口的位置和大小
        //if (!m_isMaximizedCustom) {
        //    m_restoreGeometry = geometry(); // 仅保存一次
        //}
        setCustomMaximized(true);
        ui->toolButtonMax->setVisible(false);
        ui->toolButtonRestore->setVisible(true);
        return;
    }
    // 左
    if (globalPos.x() <= avail.left() + threshold) {
        m_restoreGeometry = geometry();
        QRect leftRect(avail.left(), avail.top(), avail.width() / 2, avail.height());
        setCustomMaximized(false);
        setGeometry(leftRect);
        ui->toolButtonMax->setVisible(true);
        ui->toolButtonRestore->setVisible(false);
        return;
    }

    // 右
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
