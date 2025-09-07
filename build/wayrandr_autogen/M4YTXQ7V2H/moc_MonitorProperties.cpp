/****************************************************************************
** Meta object code from reading C++ file 'MonitorProperties.hpp'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/models/MonitorProperties.hpp"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MonitorProperties.hpp' doesn't include <QObject>."
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
struct qt_meta_tag_ZN17MonitorPropertiesE_t {};
} // unnamed namespace

template <> constexpr inline auto MonitorProperties::qt_create_metaobjectdata<qt_meta_tag_ZN17MonitorPropertiesE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MonitorProperties",
        "enabledChanged",
        "",
        "flippedChanged",
        "adaptiveSyncChanged",
        "activeResolutionIndexChanged",
        "scaleChanged",
        "positionXChanged",
        "positionYChanged",
        "transformChanged",
        "enabled",
        "flipped",
        "adaptiveSync",
        "name",
        "description",
        "resolutions",
        "activeResolutionIndex",
        "scale",
        "positionX",
        "positionY",
        "transform",
        "transformList"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'enabledChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'flippedChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'adaptiveSyncChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'activeResolutionIndexChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'scaleChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'positionXChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'positionYChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'transformChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'enabled'
        QtMocHelpers::PropertyData<bool>(10, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'flipped'
        QtMocHelpers::PropertyData<bool>(11, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'adaptiveSync'
        QtMocHelpers::PropertyData<bool>(12, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 2),
        // property 'name'
        QtMocHelpers::PropertyData<QString>(13, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'description'
        QtMocHelpers::PropertyData<QString>(14, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'resolutions'
        QtMocHelpers::PropertyData<QStringList>(15, QMetaType::QStringList, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'activeResolutionIndex'
        QtMocHelpers::PropertyData<int>(16, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 3),
        // property 'scale'
        QtMocHelpers::PropertyData<float>(17, QMetaType::Float, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 4),
        // property 'positionX'
        QtMocHelpers::PropertyData<int>(18, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 5),
        // property 'positionY'
        QtMocHelpers::PropertyData<int>(19, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 6),
        // property 'transform'
        QtMocHelpers::PropertyData<QString>(20, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 7),
        // property 'transformList'
        QtMocHelpers::PropertyData<QStringList>(21, QMetaType::QStringList, QMC::DefaultPropertyFlags | QMC::Constant),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MonitorProperties, qt_meta_tag_ZN17MonitorPropertiesE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MonitorProperties::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17MonitorPropertiesE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17MonitorPropertiesE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN17MonitorPropertiesE_t>.metaTypes,
    nullptr
} };

void MonitorProperties::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MonitorProperties *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->enabledChanged(); break;
        case 1: _t->flippedChanged(); break;
        case 2: _t->adaptiveSyncChanged(); break;
        case 3: _t->activeResolutionIndexChanged(); break;
        case 4: _t->scaleChanged(); break;
        case 5: _t->positionXChanged(); break;
        case 6: _t->positionYChanged(); break;
        case 7: _t->transformChanged(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (MonitorProperties::*)()>(_a, &MonitorProperties::enabledChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (MonitorProperties::*)()>(_a, &MonitorProperties::flippedChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (MonitorProperties::*)()>(_a, &MonitorProperties::adaptiveSyncChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (MonitorProperties::*)()>(_a, &MonitorProperties::activeResolutionIndexChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (MonitorProperties::*)()>(_a, &MonitorProperties::scaleChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (MonitorProperties::*)()>(_a, &MonitorProperties::positionXChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (MonitorProperties::*)()>(_a, &MonitorProperties::positionYChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (MonitorProperties::*)()>(_a, &MonitorProperties::transformChanged, 7))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->isEnabled(); break;
        case 1: *reinterpret_cast<bool*>(_v) = _t->isFlipped(); break;
        case 2: *reinterpret_cast<bool*>(_v) = _t->isAdaptiveSync(); break;
        case 3: *reinterpret_cast<QString*>(_v) = _t->getName(); break;
        case 4: *reinterpret_cast<QString*>(_v) = _t->getDescription(); break;
        case 5: *reinterpret_cast<QStringList*>(_v) = _t->getResolutions(); break;
        case 6: *reinterpret_cast<int*>(_v) = _t->getActiveResolutionIndex(); break;
        case 7: *reinterpret_cast<float*>(_v) = _t->getScale(); break;
        case 8: *reinterpret_cast<int*>(_v) = _t->getPositionX(); break;
        case 9: *reinterpret_cast<int*>(_v) = _t->getPositionY(); break;
        case 10: *reinterpret_cast<QString*>(_v) = _t->getTransform(); break;
        case 11: *reinterpret_cast<QStringList*>(_v) = _t->getTransformList(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setEnabled(*reinterpret_cast<bool*>(_v)); break;
        case 1: _t->setFlipped(*reinterpret_cast<bool*>(_v)); break;
        case 2: _t->setAdaptiveSync(*reinterpret_cast<bool*>(_v)); break;
        case 6: _t->setActiveResolutionIndex(*reinterpret_cast<int*>(_v)); break;
        case 7: _t->setScale(*reinterpret_cast<float*>(_v)); break;
        case 8: _t->setPositionX(*reinterpret_cast<int*>(_v)); break;
        case 9: _t->setPositionY(*reinterpret_cast<int*>(_v)); break;
        case 10: _t->setTransform(*reinterpret_cast<QString*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *MonitorProperties::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MonitorProperties::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN17MonitorPropertiesE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int MonitorProperties::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void MonitorProperties::enabledChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MonitorProperties::flippedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MonitorProperties::adaptiveSyncChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void MonitorProperties::activeResolutionIndexChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void MonitorProperties::scaleChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void MonitorProperties::positionXChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void MonitorProperties::positionYChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void MonitorProperties::transformChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}
QT_WARNING_POP
