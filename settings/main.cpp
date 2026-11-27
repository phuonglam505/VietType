// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QCursor>
#include <QDialog>
#include <QFont>
#include <QFormLayout>
#include <QGroupBox>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QIcon>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScreen>
#include <QStringList>
#include <QVBoxLayout>
#include <QWinEventNotifier>
#include <windows.h>

namespace {
bool sendLine(QByteArray line)
{
    line.append('\n');
    DWORD written = 0;
    return WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), line.constData(),
                     static_cast<DWORD>(line.size()), &written, nullptr)
        && written == static_cast<DWORD>(line.size());
}

void sendCommand(const char *command, QJsonValue value = {}, WId owner = 0)
{
    QJsonObject message{{QStringLiteral("cmd"), QString::fromLatin1(command)}};
    if (!value.isNull())
        message.insert(QStringLiteral("v"), value);
    if (owner)
        message.insert(QStringLiteral("owner"), static_cast<qint64>(owner));
    if (!sendLine(QJsonDocument(message).toJson(QJsonDocument::Compact)))
        QCoreApplication::quit();
}

enum class ActionIcon { Check, Convert, Info, Macros, Keyboard, Exclude, Charset };

QIcon actionIcon(ActionIcon icon, const QColor &color)
{
    QPainterPath path;
    switch (icon) {
    case ActionIcon::Check:
        path.moveTo(3.5, 8); path.lineTo(6.5, 11); path.lineTo(12.5, 5);
        break;
    case ActionIcon::Convert:
        path.moveTo(2.5, 5); path.lineTo(13, 5);
        path.moveTo(10.5, 2.5); path.lineTo(13, 5); path.lineTo(10.5, 7.5);
        path.moveTo(13.5, 11); path.lineTo(3, 11);
        path.moveTo(5.5, 8.5); path.lineTo(3, 11); path.lineTo(5.5, 13.5);
        break;
    case ActionIcon::Info:
        path.addEllipse(QRectF(2, 2, 12, 12));
        path.moveTo(8, 7); path.lineTo(8, 11);
        path.addEllipse(QRectF(7.8, 4.8, 0.4, 0.4));
        break;
    case ActionIcon::Macros:
        path.addRoundedRect(QRectF(3.5, 2, 9, 12), 1, 1);
        path.moveTo(6, 6); path.lineTo(10, 6);
        path.moveTo(6, 9); path.lineTo(10, 9);
        break;
    case ActionIcon::Keyboard:
        path.addRoundedRect(QRectF(1.5, 4, 13, 8), 1, 1);
        path.moveTo(4, 7); path.lineTo(4.5, 7);
        path.moveTo(7.75, 7); path.lineTo(8.25, 7);
        path.moveTo(11.5, 7); path.lineTo(12, 7);
        path.moveTo(5, 9.5); path.lineTo(11, 9.5);
        break;
    case ActionIcon::Exclude:
        path.addEllipse(QRectF(2, 2, 12, 12));
        path.moveTo(3.8, 3.8); path.lineTo(12.2, 12.2);
        break;
    case ActionIcon::Charset:
        path.addRoundedRect(QRectF(2, 2.5, 12, 11), 1, 1);
        path.moveTo(2, 5.5); path.lineTo(14, 5.5);
        path.moveTo(5.5, 11); path.lineTo(8, 7.5); path.lineTo(10.5, 11);
        break;
    }

    QPixmap image(32, 32);
    image.setDevicePixelRatio(2);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(path);
    painter.end();
    return QIcon(image);
}

class ActionButton : public QPushButton {
public:
    ActionButton(const QString &label, ActionIcon symbol, QWidget *parent)
        : QPushButton(label, parent),
          normalIcon(actionIcon(symbol, QColor("#1a1a1a"))),
          hoverIcon(actionIcon(symbol, Qt::white))
    {
        setIcon(normalIcon);
        setIconSize(QSize(16, 16));
        setAutoDefault(false);
        setFocusPolicy(Qt::TabFocus);
    }

protected:
    void enterEvent(QEnterEvent *event) override
    {
        setIcon(hoverIcon);
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        setIcon(normalIcon);
        QPushButton::leaveEvent(event);
    }

private:
    const QIcon normalIcon;
    const QIcon hoverIcon;
};

class SettingsDialog : public QDialog {
public:
    explicit SettingsDialog(const QJsonObject &settings)
    {
        setWindowTitle(tr("Cài đặt VietType"));
        setMinimumWidth(480);
        setStyleSheet(QStringLiteral(R"(
            QPushButton {
                background-color: #ffffff;
                color: #1a1a1a;
                border: 1px solid #d1d1d1;
                border-radius: 4px;
                outline: none;
                padding: 5px 10px;
                min-height: 16px;
            }
            QPushButton:focus { border-color: #0067c0; }
            QPushButton:enabled:hover { background-color: #78213f; border-color: #78213f; color: white; }
            QPushButton:pressed { background-color: #dedede; }
            QPushButton:enabled:hover:pressed { background-color: #59152e; border-color: #59152e; color: white; }
            QPushButton:disabled { color: #777777; background-color: #f3f3f3; }
            QCheckBox::indicator:checked {
                width: 16px;
                height: 16px;
                image: url(:/checkbox-checked.png);
            }
        )"));
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(10, 10, 10, 12);
        layout->setSpacing(9);

        auto *title = new QLabel(QStringLiteral("VietType"), this);
        QFont titleFont = title->font();
        titleFont.setPointSize(17);
        titleFont.setBold(true);
        title->setFont(titleFont);
        auto *header = new QHBoxLayout;
        auto *logo = new QLabel(this);
        logo->setPixmap(QApplication::windowIcon().pixmap(QSize(28, 28), devicePixelRatioF()));
        header->addWidget(logo);
        header->addWidget(title);
        header->addStretch();
        header->addWidget(new QLabel(settings.value("version").toString(), this));
        layout->addLayout(header);

        auto *upper = new QHBoxLayout;
        auto *controls = new QVBoxLayout;
        auto *actions = new QVBoxLayout;
        upper->addLayout(controls, 1);
        upper->addLayout(actions);
        layout->addLayout(upper);

        auto *typing = new QGroupBox(tr("Bộ gõ"), this);
        auto *form = new QFormLayout(typing);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        auto addChoice = [&](const QString &label, const char *command, const char *choices) {
            auto *combo = new QComboBox(typing);
            for (const auto &item : settings.value(choices).toArray())
                combo->addItem(item.toString());
            combo->setCurrentIndex(settings.value(command).toInt());
            form->addRow(label, combo);
            connect(combo, &QComboBox::currentIndexChanged, this,
                    [command](int index) { sendCommand(command, index); });
        };
        addChoice(tr("&Bảng mã"), "cs", "charsets");
        addChoice(tr("&Kiểu gõ"), "im", "methods");
        controls->addWidget(typing);

        auto addCheck = [&](QVBoxLayout *group, const QString &label, const char *command) {
            auto *box = new QCheckBox(label);
            box->setChecked(settings.value(command).toBool());
            group->addWidget(box);
            connect(box, &QCheckBox::toggled, this, [this, command](bool checked) {
                sendCommand(command, checked);
                if (qstrcmp(command, "admin") == 0)
                    close();
            });
        };
        auto *options = new QGroupBox(tr("Tùy chọn gõ"), this);
        auto *optionsLayout = new QVBoxLayout(options);
        addCheck(optionsLayout, tr("Kiểm tra chính tả"), "spell");
        addCheck(optionsLayout, tr("Bỏ dấu tự do"), "free");
        addCheck(optionsLayout, tr("Đặt dấu kiểu mới (oà, uý)"), "modern");
        addCheck(optionsLayout, tr("Tiếng Tây Nguyên (Êđê)"), "ede");
        addCheck(optionsLayout, tr("Bật gõ tắt"), "macro");
        controls->addWidget(options);

        auto *lower = new QHBoxLayout;
        layout->addLayout(lower);
        auto *system = new QGroupBox(tr("Hệ thống"), this);
        auto *systemLayout = new QVBoxLayout(system);
        addCheck(systemLayout, tr("Khởi động cùng Windows"), "autostart");
        addCheck(systemLayout, tr("Chạy với quyền quản trị"), "admin");
        addCheck(systemLayout, tr("Tạo lối tắt trên Desktop"), "desktop_shortcut");
        lower->addWidget(system);

        auto *tools = new QGroupBox(tr("Công cụ"), this);
        auto *toolLayout = new QGridLayout(tools);
        auto makeButton = [&](const QString &label, ActionIcon icon) {
            auto *button = new ActionButton(label, icon, this);
            auto *shadow = new QGraphicsDropShadowEffect(button);
            shadow->setBlurRadius(4);
            shadow->setOffset(0, 1);
            shadow->setColor(QColor(0, 0, 0, 20));
            button->setGraphicsEffect(shadow);
            return button;
        };
        auto addTool = [&](int row, int column, const QString &label, const char *command,
                           ActionIcon icon) {
            auto *button = makeButton(label, icon);
            toolLayout->addWidget(button, row, column);
            connect(button, &QPushButton::clicked, this, [this, command] {
                sendCommand(command, {}, winId());
            });
        };
        addTool(0, 0, tr("Quản lý gõ tắt"), "macrotbl", ActionIcon::Macros);
        addTool(0, 1, tr("Gán phím"), "hotkey", ActionIcon::Keyboard);
        addTool(1, 0, tr("Loại trừ ứng dụng"), "blacklist", ActionIcon::Exclude);
        addTool(1, 1, tr("Bảng mã theo ứng dụng"), "appcs", ActionIcon::Charset);
        lower->addWidget(tools, 1);

        auto *done = makeButton(tr("Đóng"), ActionIcon::Check);
        done->setDefault(true);
        actions->addWidget(done);
        connect(done, &QPushButton::clicked, this, &QDialog::accept);
        auto *converter = makeButton(tr("Chuyển mã"), ActionIcon::Convert);
        actions->addWidget(converter);
        connect(converter, &QPushButton::clicked, this, [this] { sendCommand("converter", {}, winId()); });
        auto *about = makeButton(tr("Giới thiệu"), ActionIcon::Info);
        actions->addWidget(about);
        connect(about, &QPushButton::clicked, this, [this] { sendCommand("info", {}, winId()); });
        actions->addStretch();
    }
};
} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("VietType"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/v.ico")));
    const QStringList args = app.arguments();
    if (args.size() != 3)
        return 1;

    const QJsonDocument initial = QJsonDocument::fromJson(args.at(1).toUtf8());
    bool validPid = false;
    const DWORD enginePid = args.at(2).toUInt(&validPid);
    if (!initial.isObject() || !validPid || enginePid == 0)
        return 1;

    HANDLE engine = OpenProcess(SYNCHRONIZE, FALSE, enginePid);
    if (!engine)
        return 1;

    int result = 1;
    {
        QWinEventNotifier engineExit(engine);
        QObject::connect(&engineExit, &QWinEventNotifier::activated, &app, &QApplication::quit);
        SettingsDialog dialog(initial.object());
        if (sendLine("ready")) {
            auto *screen = QGuiApplication::screenAt(QCursor::pos());
            if (!screen)
                screen = QGuiApplication::primaryScreen();
            if (screen) {
                dialog.adjustSize();
                QRect frame = dialog.frameGeometry();
                frame.moveCenter(screen->availableGeometry().center());
                dialog.move(frame.topLeft());
            }
            dialog.show();
            result = app.exec();
        }
    }
    CloseHandle(engine);
    return result;
}
