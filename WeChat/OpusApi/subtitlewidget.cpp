#include "subtitlewidget.h"
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>

SubtitleItem::SubtitleItem(const QString& speaker, const QString& text, QWidget* parent)
    : QWidget(parent)
    , m_createTime(QDateTime::currentDateTime())
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(16, 8, 16, 8);
    layout->setSpacing(10);

    // 说话人标签
    m_speakerLabel = new QLabel(speaker + ":", this);
    m_speakerLabel->setStyleSheet(
        "QLabel {"
        "color: #00BFFF;"  // 亮蓝色
        "font-weight: bold;"
        "font-size: 16px;"
        "font-family: '微软雅黑';"
        "}"
    );

    // 文本标签
    m_textLabel = new QLabel(text, this);
    m_textLabel->setStyleSheet(
        "QLabel {"
        "color: white;"
        "font-size: 16px;"
        "font-family: '微软雅黑';"
        "}"
    );
    m_textLabel->setWordWrap(true);

    layout->addWidget(m_speakerLabel);
    layout->addWidget(m_textLabel, 1);

    // 设置背景 - 半透明黑色
    setStyleSheet(
        "SubtitleItem {"
        "background-color: rgba(0, 0, 0, 180);"  // 70%不透明度
        "border-radius: 8px;"
        "margin: 4px 0;"
        "}"
    );

    // 添加阴影效果
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(10);
    shadow->setOffset(0, 2);
    shadow->setColor(QColor(0, 0, 0, 100));
    setGraphicsEffect(shadow);

    // 创建淡出动画
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(effect);

    m_fadeAnimation = new QPropertyAnimation(effect, "opacity");
    m_fadeAnimation->setDuration(500);
    m_fadeAnimation->setStartValue(1.0);
    m_fadeAnimation->setEndValue(0.0);

    connect(m_fadeAnimation, &QPropertyAnimation::finished, [this]() {
        emit fadeOutFinished();
    });
}

void SubtitleItem::startFadeOut()
{
    m_fadeAnimation->start();
}

SubtitleWidget::SubtitleWidget(QWidget *parent)
    : QWidget(parent)
    , m_maxItems(3)
    , m_displayDuration(5000)  // 默认5秒
{
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(5);
    m_layout->setContentsMargins(20, 10, 20, 20);
    m_layout->addStretch();  // 把内容推到底部

    // 设置透明背景
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("SubtitleWidget { background-color: transparent; }");

    // 定时检查过期项
    m_checkTimer = new QTimer(this);
    m_checkTimer->setInterval(1000);  // 每秒检查一次
    connect(m_checkTimer, &QTimer::timeout, this, &SubtitleWidget::checkItemsTimeout);
    m_checkTimer->start();
}

void SubtitleWidget::addSubtitle(const QString& speaker, const QString& text)
{
    // 检查是否需要移除旧项
    while (m_items.size() >= m_maxItems) {
        removeOldestItem();
    }

    // 创建新项
    SubtitleItem* item = new SubtitleItem(speaker, text, this);
    item->setProperty("createTime", QDateTime::currentDateTime());
    m_items.append(item);
    m_layout->insertWidget(m_layout->count() - 1, item);  // 在stretch之前插入

    // 淡入效果
    QGraphicsOpacityEffect* effect = qobject_cast<QGraphicsOpacityEffect*>(item->graphicsEffect());
    if (effect) {
        effect->setOpacity(0);
        QPropertyAnimation* fadeIn = new QPropertyAnimation(effect, "opacity");
        fadeIn->setDuration(300);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);
        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
    }

    connect(item, &SubtitleItem::fadeOutFinished, [this, item]() {
        m_items.removeOne(item);
        m_layout->removeWidget(item);
        item->deleteLater();
    });
}

void SubtitleWidget::removeOldestItem()
{
    if (!m_items.isEmpty()) {
        SubtitleItem* item = m_items.takeFirst();
        item->startFadeOut();
    }
}

void SubtitleWidget::checkItemsTimeout()
{
    QDateTime now = QDateTime::currentDateTime();

    // 创建要移除的项目列表副本，避免在遍历时修改列表
    QList<SubtitleItem*> itemsToRemove;

    for (SubtitleItem* item : m_items) {
        QDateTime createTime = item->property("createTime").toDateTime();
        if (createTime.isValid() && createTime.msecsTo(now) > m_displayDuration) {
            itemsToRemove.append(item);
        }
    }

    // 移除超时的项目
    for (SubtitleItem* item : itemsToRemove) {
        m_items.removeOne(item);
        item->startFadeOut();
    }
}
