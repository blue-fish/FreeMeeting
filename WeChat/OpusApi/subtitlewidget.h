#ifndef SUBTITLEWIDGET_H
#define SUBTITLEWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QTimer>
#include <QPropertyAnimation>
#include <QLabel>
#include <QDateTime>
#include <QGraphicsDropShadowEffect>

class SubtitleItem : public QWidget
{
    Q_OBJECT
public:
    explicit SubtitleItem(const QString& speaker, const QString& text, QWidget* parent = nullptr);
    void startFadeOut();

signals:
    void fadeOutFinished();

private:
    QLabel* m_speakerLabel;
    QLabel* m_textLabel;
    QDateTime m_createTime;
    QPropertyAnimation* m_fadeAnimation;
};

class SubtitleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SubtitleWidget(QWidget *parent = nullptr);

    // 添加字幕
    void addSubtitle(const QString& speaker, const QString& text);

    // 设置最大显示条数
    void setMaxItems(int count) { m_maxItems = count; }

    // 设置显示时长（毫秒）
    void setDisplayDuration(int ms) { m_displayDuration = ms; }

private:
    void removeOldestItem();
    void checkItemsTimeout();

private:
    QVBoxLayout* m_layout;
    QList<SubtitleItem*> m_items;
    int m_maxItems;
    int m_displayDuration;
    QTimer* m_checkTimer;
};

#endif // SUBTITLEWIDGET_H
