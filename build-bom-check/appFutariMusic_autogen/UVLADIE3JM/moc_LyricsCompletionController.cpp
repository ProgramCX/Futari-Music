/****************************************************************************
** Meta object code from reading C++ file 'LyricsCompletionController.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../Qt/FutariMusic/src/LyricsCompletionController.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'LyricsCompletionController.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN26LyricsCompletionControllerE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN26LyricsCompletionControllerE = QtMocHelpers::stringData(
    "LyricsCompletionController",
    "changed",
    "",
    "errorOccurred",
    "message",
    "completionFinished",
    "reset",
    "enabled",
    "scanServerSongs",
    "type",
    "scanDirectory",
    "directory",
    "selectFile",
    "row",
    "fileIndex",
    "submit",
    "rows",
    "QVariantList",
    "fileNames",
    "busy",
    "summary",
    "selectedCount",
    "candidateCount",
    "pendingCount",
    "completed"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN26LyricsCompletionControllerE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       8,   82, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   62,    2, 0x06,    9 /* Public */,
       3,    1,   63,    2, 0x06,   10 /* Public */,
       5,    0,   66,    2, 0x06,   12 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       6,    1,   67,    2, 0x02,   13 /* Public */,
       8,    1,   70,    2, 0x02,   15 /* Public */,
      10,    1,   73,    2, 0x02,   17 /* Public */,
      12,    2,   76,    2, 0x02,   19 /* Public */,
      15,    0,   81,    2, 0x02,   22 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::QUrl,   11,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   13,   14,
    QMetaType::Void,

 // properties: name, type, flags, notifyId, revision
      16, 0x80000000 | 17, 0x00015009, uint(0), 0,
      18, QMetaType::QStringList, 0x00015001, uint(0), 0,
      19, QMetaType::Bool, 0x00015001, uint(0), 0,
      20, QMetaType::QString, 0x00015001, uint(0), 0,
      21, QMetaType::Int, 0x00015001, uint(0), 0,
      22, QMetaType::Int, 0x00015001, uint(0), 0,
      23, QMetaType::Int, 0x00015001, uint(0), 0,
      24, QMetaType::Int, 0x00015001, uint(0), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject LyricsCompletionController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN26LyricsCompletionControllerE.offsetsAndSizes,
    qt_meta_data_ZN26LyricsCompletionControllerE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN26LyricsCompletionControllerE_t,
        // property 'rows'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'fileNames'
        QtPrivate::TypeAndForceComplete<QStringList, std::true_type>,
        // property 'busy'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'summary'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'selectedCount'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'candidateCount'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'pendingCount'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'completed'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<LyricsCompletionController, std::true_type>,
        // method 'changed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'errorOccurred'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'completionFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'reset'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'scanServerSongs'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'scanDirectory'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QUrl &, std::false_type>,
        // method 'selectFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'submit'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void LyricsCompletionController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<LyricsCompletionController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->changed(); break;
        case 1: _t->errorOccurred((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->completionFinished(); break;
        case 3: _t->reset((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 4: _t->scanServerSongs((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->scanDirectory((*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[1]))); break;
        case 6: _t->selectFile((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 7: _t->submit(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (LyricsCompletionController::*)();
            if (_q_method_type _q_method = &LyricsCompletionController::changed; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (LyricsCompletionController::*)(const QString & );
            if (_q_method_type _q_method = &LyricsCompletionController::errorOccurred; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (LyricsCompletionController::*)();
            if (_q_method_type _q_method = &LyricsCompletionController::completionFinished; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QVariantList*>(_v) = _t->rows(); break;
        case 1: *reinterpret_cast< QStringList*>(_v) = _t->fileNames(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->busy(); break;
        case 3: *reinterpret_cast< QString*>(_v) = _t->summary(); break;
        case 4: *reinterpret_cast< int*>(_v) = _t->selectedCount(); break;
        case 5: *reinterpret_cast< int*>(_v) = _t->candidateCount(); break;
        case 6: *reinterpret_cast< int*>(_v) = _t->pendingCount(); break;
        case 7: *reinterpret_cast< int*>(_v) = _t->completed(); break;
        default: break;
        }
    }
}

const QMetaObject *LyricsCompletionController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LyricsCompletionController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN26LyricsCompletionControllerE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int LyricsCompletionController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void LyricsCompletionController::changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void LyricsCompletionController::errorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void LyricsCompletionController::completionFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
