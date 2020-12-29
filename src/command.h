#ifndef GREEN_COMMAND_H
#define GREEN_COMMAND_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(Device)

class Activity : public QObject
{
    Q_OBJECT
public:
    enum class Status {
        Pending,
        Finished,
        Failed,
    };
    Activity(Device* device);
    Device* device() const;
    virtual void exec() = 0;
protected:
    Status status() const;
    void finish();
    void fail();
signals:
    void finished();
    void failed();
private:
    Device* const m_device;
    Status m_status{Status::Pending};
};

template <typename T>
class Command2 : public Activity
{
public:
    Command2(Device* device) : Activity(device)
    {
    }
    T result() const { return m_result; };
    void setResult(const T& result)
    {
        m_result = result;
        finish();
    }
private:
    T m_result;
};


class CommandBatch;

class Command : public QObject
{
    Q_OBJECT
public:
    Command(CommandBatch* batch = nullptr);
    virtual ~Command();
public slots:
    virtual void exec() = 0;
signals:
    void error();
    void finished();
};

class DeviceCommand : public Command
{
public:
    DeviceCommand(Device* device, CommandBatch* batch = nullptr)
        : Command(batch)
        , m_device(device)
    {
    }
    virtual QByteArray payload() const = 0;
    virtual bool parse(const QByteArray& data);
    virtual bool parse(QDataStream& stream) { Q_UNUSED(stream); Q_UNIMPLEMENTED(); Q_UNREACHABLE(); };
    int readHIDReport(Device* device, QDataStream& stream);
    bool readAPDUResponse(Device* device, int length, QDataStream& stream);

    virtual void exec() override;
    Device* const m_device;
    uint16_t length;
    uint16_t offset;
    QByteArray buf;
    QByteArray m_response;
};

class CommandBatch : public Command
{
    Q_OBJECT
public:
    void add(Command* command) { m_commands.append(command); }
    void exec() override {
        if (m_commands.isEmpty()) return emit finished();
        auto next = m_commands.takeFirst();
        connect(next, &Command::finished, this, &Command::exec, Qt::QueuedConnection);
        connect(next, &Command::error, this, &Command::error);
        next->exec();
    }
private:
    QList<Command*> m_commands;
};

class GenericCommand : public DeviceCommand
{
    const QByteArray m_data;
public:
    GenericCommand(Device* device, const QByteArray& data, CommandBatch* batch = nullptr)
        : DeviceCommand(device, batch)
        , m_data(data) {}
    QByteArray payload() const override { return m_data; }
    virtual bool parse(QDataStream& stream) override { return true; };
};

class GetAppNameCommand : public DeviceCommand
{
public:
    GetAppNameCommand(Device* device, CommandBatch* batch = nullptr)
        : DeviceCommand(device, batch)
    {}
    QByteArray payload() const override;
    bool parse(QDataStream& stream) override;
    QString m_name;
    QString m_version;
};

class GetFirmwareCommand : public DeviceCommand
{
public:
    GetFirmwareCommand(Device* device, CommandBatch* batch = nullptr)
        : DeviceCommand(device, batch)
    {}
    QByteArray payload() const override;
    bool parse(QDataStream& stream) override;
};


#endif // GREEN_COMMAND_H
