#include "device.h"
#include "device_p.h"
#include "ga.h"
#include "handler.h"
#include "json.h"
#include "network.h"
#include "networkmanager.h"
#include "util.h"
#include "wallet.h"
#include "walletmanager.h"


#define BTCHIP_CLA              0xe0
#define BTCHIP_CLA_COMMON_SDK   0xb0

#define BTCHIP_INS_GET_APP_NAME_AND_VERSION         0x01
#define BTCHIP_INS_GET_LIQUID_BLINDING_KEY          0xe2
#define BTCHIP_INS_HASH_INPUT_START                 0x44
#define BTCHIP_INS_HASH_INPUT_FINALIZE_FULL         0x4a
#define BTCHIP_INS_HASH_SIGN                        0x48
#define BTCHIP_INS_GET_WALLET_PUBLIC_KEY            0x40
#define BTCHIP_INS_GET_LIQUID_BLINDING_KEY          0xe2
#define BTCHIP_INS_GET_LIQUID_NONCE                 0xe4
#define BTCHIP_INS_GET_LIQUID_COMMITMENTS           0xe0
#define BTCHIP_INS_GET_LIQUID_BLINDING_FACTOR       0xe8
#define BTCHIP_INS_GET_LIQUID_ISSUANCE_INFORMATION  0xe6
#define BTCHIP_INS_GET_FIRMWARE_VERSION             0xc4
#define BTCHIP_INS_SIGN_MESSAGE                     0x4e


extern "C" {
struct words;
struct ext_key;
int bip32_key_free(const struct ext_key *hdkey);
int bip32_key_init_alloc(uint32_t version,
                         uint32_t depth,
                         uint32_t child_num,
                         const unsigned char *chain_code,
                         size_t chain_code_len,
                         const unsigned char *pub_key,
                         size_t pub_key_len,
                         const unsigned char *priv_key,
                         size_t priv_key_len,
                         const unsigned char *hash160,
                         size_t hash160_len,
                         const unsigned char *parent160,
                         size_t parent160_len,
                         struct ext_key **output);
int bip32_key_to_base58(const struct ext_key *hdkey, uint32_t flags, char **output);
int wally_free_string(char *str);

// From wally_core.h
#define WALLY_OK      0

}

template <typename T> struct Varint { T v; };
template <typename T> Varint<T> varint(T v) { return {v}; }
QDataStream& operator<<(QDataStream& out, const Varint<uint32_t>& v)
{
    if (v.v < 0xfd) return out << uint8_t(v.v & 0xff);
    if (v.v < 0xffff) return out << uint8_t(0xfd) << uint8_t(v.v & 0xff) << uint8_t((v.v >> 8) & 0xff);
    return out << uint8_t(0xfe) << v.v;
}

QByteArray apdu(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, const QByteArray& data = QByteArray())
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    Q_ASSERT(data.length() < 256);
    stream << cla << ins << p1 << p2 << uint8_t(data.length());
    return result + data;
}



Device::Device(DevicePrivate* d, QObject* parent)
    : QObject(parent)
    , d(d)
{
    d->q = this;
}

Device::~Device()
{
    delete d;
}

Device::Type Device::type() const
{
    return d->type;
}

bool Device::isBusy() const
{
    return !d->queue.empty();
}

QString Device::appName() const
{
    return d->app_name;
}

void Device::setAppName(const QString& app_name)
{
    if (d->app_name == app_name) return;
    d->app_name = app_name;
    emit appNameChanged();

    QString id;
    if (app_name == "Bitcoin") id = "mainnet";
    if (app_name == "Bitcoin Test") id = "testnet";
    auto network = NetworkManager::instance()->network(id);
    if (!network) return;
    auto controller = new LedgerLoginController(this, network);
    controller->login();
}

void Device::exchange(DeviceCommand* command)
{
    d->exchange(command);
}

DeviceCommand* Device::exchange(const QByteArray& data) {
    auto command = new GenericCommand(this, data);
    command->exec();
    return command;
}

GetWalletPublicKeyCommand *Device::getWalletPublicKey(Network *network, const QVector<uint32_t> &path)
{
    auto command = new GetWalletPublicKeyCommand(this, network, path);
    command->exec();
    return command;
}

QByteArray inputBytes(const QJsonObject& input, bool is_segwit)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    QByteArray txhash_hex = ParseByteArray(input.value("txhash"));
    for (int i = txhash_hex.size() - 1; i >= 0; --i) {
        stream << uint8_t(txhash_hex.at(i));
    }
    uint32_t pt_idx = input.value("pt_idx").toInt();
    stream << pt_idx;
    if (is_segwit) {
        // TODO ensure "satoshi" is double, not an object
        const quint64 satoshi = ParseSatoshi(input.value("satoshi"));
        stream << satoshi;
    }
    return data;
}

int varIntSize(int i) {
    // if negative, it's actually a very large unsigned long value
    if (i < 0) return 9; // 1 marker + 8 data bytes
    if (i< 253) return 1; // 1 data byte
    if (i <= 0xFFFF) return 3; // 1 marker + 2 data bytes
    if (i <= 0xFFFFFFFF) return 5; // 1 marker + 4 data bytes
    return 9; // 1 marker + 8 data bytes
}

void varInt(QDataStream& stream, int64_t i)
{
    switch (varIntSize(i)) {
    case 1:
        stream << uint8_t(i);
        break;
    case 3:
        stream << uint8_t(253) << uint8_t(i & 0xff) << uint8_t((i >> 8) & 0xff);
        break;
    case 5:
        stream << uint8_t(254) << uint32_t(i);
        break;
    default:
        stream << uint8_t(255) << qint64(i);
    }
}

QByteArray pathToData(const QVector<uint32_t>& path)
{
    Q_ASSERT(path.size() <= 10);
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << uint8_t(path.size());
    for (int32_t p : path) stream << uint32_t(p);
    return data;
}

QByteArray outputBytes(const QJsonArray outputs)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    varInt(stream, outputs.size());
    for (const auto& out : outputs) {
        // TODO ensure "satoshi" is double, not an object
        const quint64 satoshi = ParseSatoshi(out["satoshi"]);
        const QByteArray script = ParseByteArray(out["script"]);
        stream << satoshi;
        varInt(stream, script.size());
        stream.writeRawData(script.data(), script.size());
    }
    return data;
}

SignTransactionCommand* Device::signTransaction(const QJsonObject& required_data)
{
    Q_ASSERT(required_data.value("action").toString() == "sign_tx");

    auto transaction = required_data.value("transaction").toObject();
    auto inputs = required_data.value("signing_inputs").toArray();
    auto outputs = required_data.value("transaction_outputs").toArray();
    // these are unused
    uint32_t version = transaction.value("transaction_version").toInt();
    uint32_t locktime = transaction.value("transaction_locktime").toInt();

    bool new_transaction = true;
    bool segwit = true;
    int input_index = 0;
    QList<Input> used_inputs;
    for (auto i : inputs) {
        auto input = i.toObject();
        Input in;
        uint32_t sequence = ParseSequence(input.value("sequence"));
        QDataStream stream(&in.sequence, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream << sequence;
        in.value = inputBytes(input, segwit);
        in.trusted = false;
        in.segwit = true;
        used_inputs.append(in);
    }
    Q_ASSERT(inputs.size() > 0 && inputs.first().toObject().contains("prevout_script"));
    const auto redeem_script = ParseByteArray(inputs.first().toObject().value("prevout_script"));

    auto command = new SignTransactionCommand(this);

    startUntrustedTransaction(version, new_transaction, input_index, used_inputs, redeem_script, true);
    auto bytes = outputBytes(outputs);
    finalizeInputFull(bytes);
    signSWInputs(command, used_inputs, inputs, version, locktime);

    return command;
}

void Device::finalizeInputFull(const QByteArray& data)
{
    qDebug() << "finalizeInputFull";
    QList<QByteArray> datas;
    QByteArray x;
    x.append(uint8_t(0));
    datas.append(x);
    int offset = 0;
    while (offset < data.size()) {
        int blockLength = (data.size() - offset) > 255 ? 255 : data.size() - offset;
        datas.append(data.mid(offset, blockLength));
        offset += blockLength;
    }

    for (int i = 0; i < datas.size(); ++i) {
        uint8_t p1 = i == 0 ? 0xff : (i == datas.size() - 1 ? 0x80 : 0x00);
        qDebug() << "  " << i << p1 << data.toHex();
        auto c1 = exchange(apdu(BTCHIP_CLA, BTCHIP_INS_HASH_INPUT_FINALIZE_FULL, p1, 0x00, datas.at(i)));
        connect(c1, &Command::finished, [i, c1, datas] {
        });
    }
        /*
        let allObservables = datas
        .enumerated()
        .map { item -> Observable<Data> in
            return Observable.just(item)
                .flatMap { item -> Observable<Data> in
                    let p1: UInt8 = item.offset == 0 ? 0xFF : item.offset == datas.count - 1 ? 0x80 : 0x00
                    return self.exchangeAdpu(cla: self.CLA_BOLOS, ins: self.INS_HASH_INPUT_FINALIZE_FULL, p1: p1, p2: 0x00, data: item.element)
                }
                .asObservable()
                .timeoutIfNoEvent(RxTimeInterval.seconds(TIMEOUT))
                .take(1)
        }
    return Observable<Data>.concat(allObservables).reduce(Data(), accumulator: { _, element in
        element
    }).flatMap { buffer -> Observable<[String: Any]> in
        return Observable.just(self.convertResponseToOutput(buffer))
    }*/
}

void Device::signSWInputs(SignTransactionCommand* command, const QList<Input>& hwInputs, const QJsonArray& inputs, uint32_t version, uint32_t locktime)
{
    command->count = hwInputs.size();
    for (int i = 0; i < hwInputs.size(); ++i) {
        const auto& input = hwInputs.at(i);
        signSWInput(command, input, inputs.at(i).toObject(), version, locktime);
    }
//    let allObservables = hwInputs
//        .enumerated()
//        .map { hwInput -> Observable<Data> in
//            return Observable.just(hwInput.element)
//                .flatMap { _ in self.signSWInput(hwInput: hwInput.element, input: inputs[hwInput.offset], version: version, locktime: locktime) }
//                .asObservable()
//                .take(1)
//    }
//    return Observable.concat(allObservables).reduce([], accumulator: { result, element in
//        result + [element]
//    })
}

void Device::signSWInput(SignTransactionCommand* command, const Input& hwInput, const QJsonObject& input, uint32_t version, uint32_t locktime)
{
    auto script = ParseByteArray(input.value("prevout_script"));
    startUntrustedTransaction(version, false, 0, {hwInput}, script, true);
    QVector<uint32_t> user_path = ParsePath(input.value("user_path"));
    uint8_t SIGHASH_ALL = 1;
    untrustedHashSign(command, user_path, "0", locktime, SIGHASH_ALL);
}


void Device::untrustedHashSign(SignTransactionCommand* command, const QVector<uint32_t>& private_key_path, QString pin, uint32_t locktime, uint8_t sig_hash_type)
{
    auto path = pathToData(private_key_path);
    auto _pin = pin.toUtf8();
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.writeRawData(path.data(), path.size());
    stream.setByteOrder(QDataStream::BigEndian);
    stream << uint8_t(pin.size());
    stream.writeRawData(_pin.data(), _pin.size());
    stream << uint32_t(locktime) << sig_hash_type;
    qDebug("untrustedHashSign EXCHANGE");
    auto c1 = exchange(apdu(BTCHIP_CLA, BTCHIP_INS_HASH_SIGN, 0, 0, data));
    connect(c1, &Command::error, [] {
       qDebug("untrustedHashSign FAILED!!!!!!!!!");
    });
    connect(c1, &Command::finished, [command, c1] {
       qDebug("untrustedHashSign FINISHED!!");
       QByteArray signature;
       signature.append(0x30);
       signature.append(c1->m_response.mid(1));
       //signature = x;
       command->signatures.append(signature);
       if (command->signatures.size() == command->count) {
           emit command->finished();
       }
    });
}


void Device::startUntrustedTransaction(uint32_t tx_version, bool new_transaction, int64_t input_index, const QList<Input>& used_input, const QByteArray& redeem_script, bool segwit)
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    // Start building a fake transaction with the passed inputs
    stream << tx_version << varint<uint32_t>(used_input.size());
    const uint8_t p2 = new_transaction ? (segwit ? 0x02 : 0x00) : 0x80;
    auto c = exchange(apdu(BTCHIP_CLA, BTCHIP_INS_HASH_INPUT_START, 0x00, p2, data));
    connect(c, &Command::finished, [] {
        qDebug("startUntrustedTransaction OK");
    });
    hashInputs(used_input, input_index, redeem_script);
}

void Device::hashInputs(const QList<Input>& used_inputs, int64_t input_index, const QByteArray& redeem_script)
{
    for (int index = 0; index < used_inputs.size(); ++index) {
        hashInput(used_inputs.at(index), index == input_index ? redeem_script : QByteArray());
    }
}

void Device::hashInput(const Input& input, const QByteArray& script)
{
    const uint8_t first = input.segwit ? 0x02 : (input.trusted ? 0x01 : 0x00);
    const QByteArray value = QByteArray::fromHex(input.value);

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << first;
    if (input.trusted) stream << uint8_t(input.value.size());
    stream.writeRawData(input.value.data(), input.value.size());
    stream << varint<uint32_t>(script.size());

    auto c1 = exchange(apdu(BTCHIP_CLA, BTCHIP_INS_HASH_INPUT_START, 0x80, 0x00, data));
    auto seq = input.sequence;
    connect(c1, &Command::finished, [this, script, seq] {
        qDebug("HASH INPUT 1ST FINISHED");
//        QByteArray data;
//        QDataStream stream(&data, QIODevice::WriteOnly);
//        stream.setByteOrder(QDataStream::LittleEndian);

    });
    auto c2 = exchange(apdu(BTCHIP_CLA, BTCHIP_INS_HASH_INPUT_START, 0x80, 0x00, script + seq));
    connect(c2, &Command::finished, [] {
        qDebug("HASH INPUT 2ND FINISHED!");
    });
}



LedgerLoginController::LedgerLoginController(Device* device, Network* network)
    : QObject(device)
    , m_device(device)
    , m_network(network)
{
    hw_device = Json::fromObject({{
        "device", QJsonObject({
            { "name", "Ledger" },
            { "supports_arbitrary_scripts", true },
            { "supports_low_r", false }
        })
    }});

    m_wallet = new Wallet();
    m_wallet->setNetwork(m_network);
}

void LedgerLoginController::login()
{
    QJsonObject params{
        { "name", m_network->id() },
        { "log_level", "debug" },
        { "use_tor", false },
    };

    m_wallet->createSession();
    GA_connect(m_wallet->m_session, Json::fromObject(params));

    GA_register_user(m_wallet->m_session, hw_device, "", &m_register_handler);

    QJsonObject result = GA::auth_handler_get_result(m_register_handler);
    QString status = result.value("status").toString();

//    auto result = GA::process_auth([&] (GA_auth_handler** call) {
//        int err = GA_register_user(m_session, hw_device, "", call);
//        Q_ASSERT(err == GA_OK);
//    });

    Q_ASSERT(result.value("status").toString() == "resolve_code");
    Q_ASSERT(result.value("action").toString() == "get_xpubs");

    m_paths = result.value("required_data").toObject().value("paths").toArray();

    auto batch = new CommandBatch;
    for (auto path : m_paths) {
        auto cmd = new GetWalletPublicKeyCommand(m_device, m_network, ParsePath(path));
        connect(cmd, &Command::finished, [this, cmd] {
            m_xpubs.append(cmd->m_xpub);
        });
        batch->add(cmd);
    }
    connect(batch, &Command::finished, [this] {
        Q_ASSERT(m_xpubs.size() == m_paths.size());
        QJsonObject code= {{ "xpubs", m_xpubs }};
        qDebug() << "THIS IS IT" << code << m_paths;
        auto _code = QJsonDocument(code).toJson();
        GA_auth_handler_resolve_code(m_register_handler, _code.constData());
        GA_auth_handler_call(m_register_handler);


        m_paths = QJsonArray();
        m_xpubs = QJsonArray();

        login2();
    });
    batch->exec();

//    updateCurrencies();
//    reload();
//    updateConfig();
}

void LedgerLoginController::login2()
{
    int err = GA_login(m_wallet->m_session, hw_device, "", "", &m_login_handler);
    Q_ASSERT(err == GA_OK);

    auto result = GA::auth_handler_get_result(m_login_handler);

    Q_ASSERT(result.value("status").toString() == "resolve_code");
    Q_ASSERT(result.value("action").toString() == "get_xpubs");

    m_paths = result.value("required_data").toObject().value("paths").toArray();

    for (auto path : m_paths) {
        auto cmd = new GetWalletPublicKeyCommand(m_device, m_network, ParsePath(path));
        connect(cmd, &Command::finished, [this, cmd] {
            qDebug() << "FINICHED GET PUBKEY" << cmd->m_xpub;

            m_xpubs.append(cmd->m_xpub);

            if (m_xpubs.size() == m_paths.size()) {
                QJsonObject code= {{ "xpubs", m_xpubs }};
                auto _code = QJsonDocument(code).toJson();
                qDebug() << "LOGIN RESOLVE CODE" << _code.constData();
                GA_auth_handler_resolve_code(m_login_handler, _code.constData());
                qDebug() << GA::auth_handler_get_result(m_login_handler);
                GA_auth_handler_call(m_login_handler);
                auto result = GA::auth_handler_get_result(m_login_handler);
                qDebug() << "LOGIN RESULT AFTER CALL" << result;
                auto required_data = result.value("required_data").toObject();
                QByteArray message = required_data.value("message").toString().toLocal8Bit();
                QVector<uint32_t> path = ParsePath(required_data.value("path"));
                auto prepare = new SignMessageCommand(m_device, path, message);
                connect(prepare, &Command::finished, [this] {
                    auto sign = new SignMessageCommand(m_device);
                    connect(sign, &Command::finished, [this, sign] {
                        QJsonObject code = {{ "signature", QString::fromLocal8Bit(sign->signature.toHex()) }};

                        auto _code = QJsonDocument(code).toJson();
                        qDebug() << "RESOLVE LOGIN CODE" << _code.constData();
                        qDebug() << "GA_auth_handler_resolve_code" << GA_auth_handler_resolve_code(m_login_handler, _code.constData());
                        qDebug() << "RESULT" << GA::auth_handler_get_result(m_login_handler);
                        qDebug() << "GA_auth_handler_call" << GA_auth_handler_call(m_login_handler);
                        auto result = GA::auth_handler_get_result(m_login_handler);


                        Q_ASSERT(result.value("status").toString() == "resolve_code");
                        Q_ASSERT(result.value("action").toString() == "get_xpubs");

                        m_paths = result.value("required_data").toObject().value("paths").toArray();
                        m_xpubs = QJsonArray();

                        for (auto path : m_paths) {
                            auto cmd = new GetWalletPublicKeyCommand(m_device, m_network, ParsePath(path));
                            connect(cmd, &Command::finished, [this, cmd] {
                                qDebug() << "FINICHED GET PUBKEY" << cmd->m_xpub;

                                m_xpubs.append(cmd->m_xpub);

                                if (m_xpubs.size() == m_paths.size()) {
                                    QJsonObject code= {{ "xpubs", m_xpubs }};
                                    auto _code = QJsonDocument(code).toJson();
                                    qDebug() << "RESOLVE CODE" << _code.constData();
                                    GA_auth_handler_resolve_code(m_login_handler, _code.constData());
                                    qDebug() << GA::auth_handler_get_result(m_login_handler);
                                    GA_auth_handler_call(m_login_handler);
                                    qDebug() << GA::auth_handler_get_result(m_login_handler);

                                    m_wallet->m_device = m_device;
                                    m_wallet->setSession();
                                    WalletManager::instance()->addWallet(m_wallet);

                                    auto w = m_wallet;
                                    connect(m_device, &QObject::destroyed, [w] {
                                        WalletManager::instance()->removeWallet(w);
                                        delete w;
                                    });
                                }
                            });
                            cmd->exec();
                        }
                    });
                    sign->exec();
                });
                prepare->exec();
            }
        });
        cmd->exec();
    }
}

Device::Type Device::typefromVendorAndProduct(uint32_t vendor_id, uint32_t product_id)
{
    return Unknown;
}


QByteArray GetFirmwareCommand::payload() const
{
    return apdu(BTCHIP_CLA, BTCHIP_INS_GET_FIRMWARE_VERSION, 0x00, 0x00);
}

bool GetFirmwareCommand::parse(QDataStream &stream)
{
    uint8_t features, arch, fw_major, fw_minor, fw_patch, loader_major, loader_minor;
    stream >> features >> arch >> fw_major >> fw_minor >> fw_patch >> loader_major >> loader_minor;
    Q_ASSERT(arch == 0x30);
    //    0x01 : public keys are compressed (otherwise not compressed)
    //    0x02 : implementation running with screen + buttons handled by the Secure Element
    //    0x04 : implementation running with screen + buttons handled externally
    //    0x08 : NFC transport and payment extensions supported
    //    0x10 : BLE transport and low power extensions supported
    //    0x20 : implementation running on a Trusted Execution Environment
    qDebug() << features << arch << fw_major << fw_minor << fw_patch << loader_major << loader_minor;
    return true;
}

bool DeviceCommand::readAPDUResponse(Device* device, int length, QDataStream &stream)
{
    QByteArray response;
    if (length > 0) {
        response.resize(length - 2);
        stream.readRawData(response.data(), length - 2);
    }
    uint16_t sw;
    stream >> sw;
    if (sw != 0x9000) {
        qDebug() << "SW = " << sw;
        emit error();
        return false;
    }
    bool result = parse(response);
    if (result) {
        m_response = response;
        emit finished();
    }
    return result;
}

void DeviceCommand::exec()
{
    m_device->exchange(this);
}

Command::Command(CommandBatch* batch)
    : QObject(batch)
{
    if (batch) batch->add(this);
}

Command::~Command()
{

}

bool DeviceCommand::parse(const QByteArray& data)
{
    QDataStream stream(data);
    return parse(stream);
}

int DeviceCommand::readHIDReport(Device* device, QDataStream& stream)
{
    // General transport
    uint16_t channel_id;
    uint8_t command_tag;
    uint16_t index;
    stream >> channel_id >> command_tag >> index;

#define CHANNEL_DEFAULT_ID 0x0101
#define TAG_APDU 0x05
#define TAG_PING 0x02

    Q_ASSERT(channel_id == CHANNEL_DEFAULT_ID);
    if (command_tag == TAG_PING) Q_UNIMPLEMENTED();
    Q_ASSERT(command_tag == TAG_APDU);

    if (index == 0) {
        Q_ASSERT(buf.size() == 0);
        stream >> length;
        buf.resize(length);
        offset = 0;
    }

    int read = stream.readRawData(buf.data() + offset, length);
    length -= read;
    offset += read;

    if (length > 0) return 2;

    //qDebug() << "READ APDU RESPONSE" << buf.toHex();

    QDataStream s(buf);
    return readAPDUResponse(device, buf.size(), s) ? 0 : 1;
}

QByteArray GetAppNameCommand::payload() const
{
    return apdu(BTCHIP_CLA_COMMON_SDK, BTCHIP_INS_GET_APP_NAME_AND_VERSION, 0x00, 0x00);
}

bool GetAppNameCommand::parse(QDataStream& stream)
{
    uint8_t format;
    stream >> format;

    char* name = new char[256];
    char* version = new char[256];

    uint8_t name_length, version_length;

    stream >> name_length;
    stream.readRawData(name, name_length);
    stream >> version_length;
    stream.readRawData(version, version_length);

    m_name = QString::fromLocal8Bit(name, name_length);
    m_version = QString::fromLocal8Bit(version, version_length);
    return true;
}

QByteArray GetWalletPublicKeyCommand::payload() const
{
    QByteArray path;
    QDataStream s(&path, QIODevice::WriteOnly);
    s << uint8_t(m_path.size());
    for (auto p : m_path) s << uint32_t(p);
    return apdu(BTCHIP_CLA, BTCHIP_INS_GET_WALLET_PUBLIC_KEY, 0x0, 0, path);
}


QByteArray compressPublicKey(const QByteArray& pubkey)
{
    Q_ASSERT(pubkey.size() > 0);
    switch (pubkey[0]) {
    case 0x04:
        Q_ASSERT(pubkey.size() == 65);
        break;
    case 0x02:
    case 0x03:
        Q_ASSERT(pubkey.size() == 33);
        return pubkey;
    default:
        Q_UNREACHABLE();
    }
    auto type = pubkey[64] & 0x01 ? 0x03 : 0x02;
    return pubkey.mid(1, 32).prepend(type);
}

/** From wally_bip32.h */
/** Version codes for extended keys */
#define BIP32_VER_MAIN_PUBLIC  0x0488B21E
#define BIP32_VER_TEST_PUBLIC  0x043587CF

/** Indicate that we want to derive a public key in `bip32_key_from_parent` */
#define BIP32_FLAG_KEY_PUBLIC  0x1

bool GetWalletPublicKeyCommand::parse(QDataStream& stream)
{
    uint32_t version = m_network->data().value("mainnet").toBool() ? BIP32_VER_MAIN_PUBLIC : BIP32_VER_TEST_PUBLIC;

    uint8_t pubkey_len, address_len;
    stream >> pubkey_len;
    QByteArray pubkey(pubkey_len, 0);
    stream.readRawData(pubkey.data(), pubkey_len);
    stream >> address_len;
    QByteArray address(address_len, 0);
    stream.readRawData(address.data(), address_len);
    QByteArray chain_code(32, 0);

    stream.readRawData(chain_code.data(), 33);

    pubkey = compressPublicKey(pubkey);
    ext_key* k;
    int x = bip32_key_init_alloc(version, 1, 0, (const unsigned char *) chain_code.data(), chain_code.length(), (const unsigned char *) pubkey.constData(), pubkey.length(), nullptr, 0, nullptr, 0, nullptr, 0, &k);
    Q_ASSERT(x == 0);

    char* base58;
    x = bip32_key_to_base58(k, BIP32_FLAG_KEY_PUBLIC, &base58);
    Q_ASSERT(x == 0);
    bip32_key_free(k);

    qDebug() << base58;

    m_xpub = QString(base58);

    wally_free_string(base58);

    return true;
}

QByteArray SignMessageCommand::payload() const
{
    if (!m_message.isEmpty() && !m_path.isEmpty()) {
        QByteArray data;
        QDataStream s(&data, QIODevice::WriteOnly);
        s << uint8_t(m_path.size());
        for (auto p : m_path) s << uint32_t(p);
        s << uint8_t(0) << uint8_t(m_message.length());
        s.writeRawData(m_message.constData(), m_message.size());
        //qDebug() << "SIGN MESSAGE" << m_path << m_message;
        //qDebug() << "SIGN MESSAGE" << m_path << data.toHex();
        //qDebug() << "SIGN MESSAGE" << m_message.toHex();
        return apdu(BTCHIP_CLA, BTCHIP_INS_SIGN_MESSAGE, 0x0, 1, data);
    }
    Q_ASSERT(m_message.isEmpty() && m_path.isEmpty());
    QByteArray data;
    QDataStream s(&data, QIODevice::WriteOnly);
    s << uint8_t(1) << uint8_t(0);
    return apdu(BTCHIP_CLA, BTCHIP_INS_SIGN_MESSAGE, 0x80, 1, data);

}

bool SignMessageCommand::parse(QDataStream &stream)
{
    if (m_message.isEmpty() && m_path.isEmpty()) {
        uint8_t b;
        stream >> b;
        signature.append(0x30);
        while (stream.readRawData((char*) &b, 1) == 1) {
            signature.append(b);
        }
        return true;
    }

    return true;
}
