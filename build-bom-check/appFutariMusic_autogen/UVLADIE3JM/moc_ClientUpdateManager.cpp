/****************************************************************************
** Meta object code from reading C++ file 'ClientUpdateManager.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../Qt/FutariMusic/src/ClientUpdateManager.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ClientUpdateManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN19ClientUpdateManagerE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN19ClientUpdateManagerE = QtMocHelpers::stringData(
    "ClientUpdateManager",
    "updateInfoChanged",
    "",
    "autoSilentUpdateChanged",
    "checkingChanged",
    "downloadStateChanged",
    "downloadProgressChanged",
    "statusMessageChanged",
    "updatePromptRequested",
    "updateStarted",
    "release_tag",
    "updateSucceeded",
    "updateFailed",
    "message",
    "requestApplicationClose",
    "checkForUpdates",
    "startup_check",
    "startUpdate",
    "cancelDownload",
    "serverVersion",
    "currentClientVersion",
    "currentReleaseTag",
    "latestClientVersion",
    "releaseTag",
    "statusMessage",
    "updateAvailable",
    "autoSilentUpdate",
    "checking",
    "downloading",
    "installing",
    "downloadProgress",
    "progressIndeterminate"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN19ClientUpdateManagerE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
      13,  127, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      11,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  104,    2, 0x06,   14 /* Public */,
       3,    0,  105,    2, 0x06,   15 /* Public */,
       4,    0,  106,    2, 0x06,   16 /* Public */,
       5,    0,  107,    2, 0x06,   17 /* Public */,
       6,    0,  108,    2, 0x06,   18 /* Public */,
       7,    0,  109,    2, 0x06,   19 /* Public */,
       8,    0,  110,    2, 0x06,   20 /* Public */,
       9,    1,  111,    2, 0x06,   21 /* Public */,
      11,    1,  114,    2, 0x06,   23 /* Public */,
      12,    1,  117,    2, 0x06,   25 /* Public */,
      14,    0,  120,    2, 0x06,   27 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      15,    1,  121,    2, 0x02,   28 /* Public */,
      15,    0,  124,    2, 0x22,   30 /* Public | MethodCloned */,
      17,    0,  125,    2, 0x02,   31 /* Public */,
      18,    0,  126,    2, 0x02,   32 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void, QMetaType::QString,   13,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, QMetaType::Bool,   16,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags, notifyId, revision
      19, QMetaType::QString, 0x00015001, uint(0), 0,
      20, QMetaType::QString, 0x00015401, uint(-1), 0,
      21, QMetaType::QString, 0x00015401, uint(-1), 0,
      22, QMetaType::QString, 0x00015001, uint(0), 0,
      23, QMetaType::QString, 0x00015001, uint(0), 0,
      24, QMetaType::QString, 0x00015001, uint(5), 0,
      25, QMetaType::Bool, 0x00015001, uint(0), 0,
      26, QMetaType::Bool, 0x00015103, uint(1), 0,
      27, QMetaType::Bool, 0x00015001, uint(2), 0,
      28, QMetaType::Bool, 0x00015001, uint(3), 0,
      29, QMetaType::Bool, 0x00015001, uint(3), 0,
      30, QMetaType::Int, 0x00015001, uint(4), 0,
      31, QMetaType::Bool, 0x00015001, uint(4), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject ClientUpdateManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN19ClientUpdateManagerE.offsetsAndSizes,
    qt_meta_data_ZN19ClientUpdateManagerE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN19ClientUpdateManagerE_t,
        // property 'serverVersion'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'currentClientVersion'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'currentReleaseTag'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'latestClientVersion'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'releaseTag'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'statusMessage'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'updateAvailable'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'autoSilentUpdate'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'checking'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'downloading'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'installing'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'downloadProgress'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'progressIndeterminate'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ClientUpdateManager, std::true_type>,
        // method 'updateInfoChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'autoSilentUpdateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'checkingChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'downloadStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'downloadProgressChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'statusMessageChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updatePromptRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'updateSucceeded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'updateFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'requestApplicationClose'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'checkForUpdates'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'checkForUpdates'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startUpdate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'cancelDownload'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void ClientUpdateManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ClientUpdateManager *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->updateInfoChanged(); break;
        case 1: _t->autoSilentUpdateChanged(); break;
        case 2: _t->checkingChanged(); break;
        case 3: _t->downloadStateChanged(); break;
        case 4: _t->downloadProgressChanged(); break;
        case 5: _t->statusMessageChanged(); break;
        case 6: _t->updatePromptRequested(); break;
        case 7: _t->updateStarted((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->updateSucceeded((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->updateFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: _t->requestApplicationClose(); break;
        case 11: _t->checkForUpdates((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 12: _t->checkForUpdates(); break;
        case 13: _t->startUpdate(); break;
        case 14: _t->cancelDownload(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (ClientUpdateManager::*)();
            if (_q_method_type _q_method = &ClientUpdateManager::updateInfoChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (ClientUpdateManager::*)();
            if (_q_method_type _q_method = &ClientUpdateManager::autoSilentUpdateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (ClientUpdateManager::*)();
            if (_q_method_type _q_method = &ClientUpdateManager::checkingChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (ClientUpdateManager::*)();
            if (_q_method_type _q_method = &ClientUpdateManager::downloadStateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (ClientUpdateManager::*)();
            if (_q_method_type _q_method = &ClientUpdateManager::downloadProgressChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (ClientUpdateManager::*)();
            if (_q_method_type _q_method = &ClientUpdateManager::statusMessageChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _q_method_type = void (ClientUpdateManager::*)();
            if (_q_method_type _q_method = &ClientUpdateManager::updatePromptRequested; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _q_method_type = void (ClientUpdateManager::*)(const QString & );
            if (_q_method_type _q_method = &ClientUpdateManager::updateStarted; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _q_method_type = void (ClientUpdateManager::*)(const QString & );
            if (_q_method_type _q_method = &ClientUpdateManager::updateSucceeded; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _q_method_type = void (ClientUpdateManager::*)(const QString & );
            if (_q_method_type _q_method = &ClientUpdateManager::updateFailed; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _q_method_type = void (ClientUpdateManager::*)();
            if (_q_method_type _q_method = &ClientUpdateManager::requestApplicationClose; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->serverVersion(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->currentClientVersion(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->currentReleaseTag(); break;
        case 3: *reinterpret_cast< QString*>(_v) = _t->latestClientVersion(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->releaseTag(); break;
        case 5: *reinterpret_cast< QString*>(_v) = _t->statusMessage(); break;
        case 6: *reinterpret_cast< bool*>(_v) = _t->updateAvailable(); break;
        case 7: *reinterpret_cast< bool*>(_v) = _t->autoSilentUpdate(); break;
        case 8: *reinterpret_cast< bool*>(_v) = _t->checking(); break;
        case 9: *reinterpret_cast< bool*>(_v) = _t->downloading(); break;
        case 10: *reinterpret_cast< bool*>(_v) = _t->installing(); break;
        case 11: *reinterpret_cast< int*>(_v) = _t->downloadProgress(); break;
        case 12: *reinterpret_cast< bool*>(_v) = _t->progressIndeterminate(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 7: _t->setAutoSilentUpdate(*reinterpret_cast< bool*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *ClientUpdateManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ClientUpdateManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN19ClientUpdateManagerE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ClientUpdateManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void ClientUpdateManager::updateInfoChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ClientUpdateManager::autoSilentUpdateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ClientUpdateManager::checkingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ClientUpdateManager::downloadStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void ClientUpdateManager::downloadProgressChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void ClientUpdateManager::statusMessageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void ClientUpdateManager::updatePromptRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void ClientUpdateManager::updateStarted(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void ClientUpdateManager::updateSucceeded(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void ClientUpdateManager::updateFailed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void ClientUpdateManager::requestApplicationClose()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}
QT_WARNING_POP
