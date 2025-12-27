/****************************************************************************
** Meta object code from reading C++ file 'ReminderSettingsViewModel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../services/ReminderSettingsViewModel.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ReminderSettingsViewModel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.1. It"
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
struct qt_meta_tag_ZN25ReminderSettingsViewModelE_t {};
} // unnamed namespace

template <> constexpr inline auto ReminderSettingsViewModel::qt_create_metaobjectdata<qt_meta_tag_ZN25ReminderSettingsViewModelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ReminderSettingsViewModel",
        "settingsChanged",
        "",
        "lastErrorChanged",
        "reload",
        "save",
        "dayBeforeEnabled",
        "dayBeforeTime",
        "dayOfEnabled",
        "dayOfTime",
        "overdueEnabled",
        "overdueCadenceDays",
        "overdueTime",
        "quietHoursStart",
        "quietHoursEnd",
        "lastError"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'settingsChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'lastErrorChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'reload'
        QtMocHelpers::MethodData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'save'
        QtMocHelpers::MethodData<bool()>(5, 2, QMC::AccessPublic, QMetaType::Bool),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'dayBeforeEnabled'
        QtMocHelpers::PropertyData<bool>(6, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'dayBeforeTime'
        QtMocHelpers::PropertyData<QString>(7, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'dayOfEnabled'
        QtMocHelpers::PropertyData<bool>(8, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'dayOfTime'
        QtMocHelpers::PropertyData<QString>(9, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'overdueEnabled'
        QtMocHelpers::PropertyData<bool>(10, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'overdueCadenceDays'
        QtMocHelpers::PropertyData<int>(11, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'overdueTime'
        QtMocHelpers::PropertyData<QString>(12, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'quietHoursStart'
        QtMocHelpers::PropertyData<QString>(13, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'quietHoursEnd'
        QtMocHelpers::PropertyData<QString>(14, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'lastError'
        QtMocHelpers::PropertyData<QString>(15, QMetaType::QString, QMC::DefaultPropertyFlags, 1),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<ReminderSettingsViewModel, qt_meta_tag_ZN25ReminderSettingsViewModelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ReminderSettingsViewModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN25ReminderSettingsViewModelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN25ReminderSettingsViewModelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN25ReminderSettingsViewModelE_t>.metaTypes,
    nullptr
} };

void ReminderSettingsViewModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ReminderSettingsViewModel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->settingsChanged(); break;
        case 1: _t->lastErrorChanged(); break;
        case 2: _t->reload(); break;
        case 3: { bool _r = _t->save();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (ReminderSettingsViewModel::*)()>(_a, &ReminderSettingsViewModel::settingsChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (ReminderSettingsViewModel::*)()>(_a, &ReminderSettingsViewModel::lastErrorChanged, 1))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->dayBeforeEnabled(); break;
        case 1: *reinterpret_cast<QString*>(_v) = _t->dayBeforeTime(); break;
        case 2: *reinterpret_cast<bool*>(_v) = _t->dayOfEnabled(); break;
        case 3: *reinterpret_cast<QString*>(_v) = _t->dayOfTime(); break;
        case 4: *reinterpret_cast<bool*>(_v) = _t->overdueEnabled(); break;
        case 5: *reinterpret_cast<int*>(_v) = _t->overdueCadenceDays(); break;
        case 6: *reinterpret_cast<QString*>(_v) = _t->overdueTime(); break;
        case 7: *reinterpret_cast<QString*>(_v) = _t->quietHoursStart(); break;
        case 8: *reinterpret_cast<QString*>(_v) = _t->quietHoursEnd(); break;
        case 9: *reinterpret_cast<QString*>(_v) = _t->lastError(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setDayBeforeEnabled(*reinterpret_cast<bool*>(_v)); break;
        case 1: _t->setDayBeforeTime(*reinterpret_cast<QString*>(_v)); break;
        case 2: _t->setDayOfEnabled(*reinterpret_cast<bool*>(_v)); break;
        case 3: _t->setDayOfTime(*reinterpret_cast<QString*>(_v)); break;
        case 4: _t->setOverdueEnabled(*reinterpret_cast<bool*>(_v)); break;
        case 5: _t->setOverdueCadenceDays(*reinterpret_cast<int*>(_v)); break;
        case 6: _t->setOverdueTime(*reinterpret_cast<QString*>(_v)); break;
        case 7: _t->setQuietHoursStart(*reinterpret_cast<QString*>(_v)); break;
        case 8: _t->setQuietHoursEnd(*reinterpret_cast<QString*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *ReminderSettingsViewModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ReminderSettingsViewModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN25ReminderSettingsViewModelE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ReminderSettingsViewModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void ReminderSettingsViewModel::settingsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ReminderSettingsViewModel::lastErrorChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
