#include "gc_types.hpp"
#include "../core/gc_object.hpp"
#include "../core/garbage_collector.hpp"

namespace Lua {
namespace GCUtils {

    // Color checking functions (Lua 5.1 compatible)
    bool iswhite(const GCObject* o) {
        if (!o) return false;
        u8 marked = o->getMarked();
        return GCMark::testbits(marked, GCMark::WHITEBITS);
    }

    bool isblack(const GCObject* o) {
        if (!o) return false;
        return GCMark::testbit(o->getMarked(), GCMark::BLACKBIT);
    }

    bool isgray(const GCObject* o) {
        if (!o) return false;
        u8 marked = o->getMarked();
        return !GCMark::testbits(marked, GCMark::WHITEBITS) && !GCMark::testbit(marked, GCMark::BLACKBIT);
    }

    // Color manipulation functions
    void white2gray(GCObject* o) {
        if (!o) return;
        u8& marked = const_cast<u8&>(o->getMarkedRef());
        GCMark::reset2bits(marked, GCMark::WHITE0BIT, GCMark::WHITE1BIT);
    }

    void gray2black(GCObject* o) {
        if (!o) return;
        u8& marked = const_cast<u8&>(o->getMarkedRef());
        GCMark::l_setbit(marked, GCMark::BLACKBIT);
    }

    void black2gray(GCObject* o) {
        if (!o) return;
        u8& marked = const_cast<u8&>(o->getMarkedRef());
        GCMark::resetbit(marked, GCMark::BLACKBIT);
    }

    // Object state checking
    bool isfinalized(const GCObject* o) {
        if (!o) return false;
        return GCMark::testbit(o->getMarked(), GCMark::FINALIZEDBIT);
    }

    void markfinalized(GCObject* o) {
        if (!o) return;
        u8& marked = const_cast<u8&>(o->getMarkedRef());
        GCMark::l_setbit(marked, GCMark::FINALIZEDBIT);
    }

    bool isfixed(const GCObject* o) {
        if (!o) return false;
        return GCMark::testbit(o->getMarked(), GCMark::FIXEDBIT);
    }

    void setfixed(GCObject* o) {
        if (!o) return;
        u8& marked = const_cast<u8&>(o->getMarkedRef());
        GCMark::l_setbit(marked, GCMark::FIXEDBIT);
    }

    void unsetfixed(GCObject* o) {
        if (!o) return;
        u8& marked = const_cast<u8&>(o->getMarkedRef());
        GCMark::resetbit(marked, GCMark::FIXEDBIT);
    }

    // Weak table checking
    bool hasweakkeys(const GCObject* o) {
        if (!o) return false;
        return GCMark::testbit(o->getMarked(), GCMark::KEYWEAKBIT);
    }

    bool hasweakvalues(const GCObject* o) {
        if (!o) return false;
        return GCMark::testbit(o->getMarked(), GCMark::VALUEWEAKBIT);
    }

    void setweakkeys(GCObject* o) {
        if (!o) return;
        u8& marked = const_cast<u8&>(o->getMarkedRef());
        GCMark::l_setbit(marked, GCMark::KEYWEAKBIT);
    }

    void setweakvalues(GCObject* o) {
        if (!o) return;
        u8& marked = const_cast<u8&>(o->getMarkedRef());
        GCMark::l_setbit(marked, GCMark::VALUEWEAKBIT);
    }

    // Functions that need GarbageCollector implementation
    bool isdead(const GarbageCollector* g, const GCObject* o) {
        if (!g || !o) return true;

        // An object is dead if it's white and the current white is different
        u8 marked = o->getMarked();
        u8 currentWhite = g->getCurrentWhiteBits();

        // Object is dead if it's white but not the current white
        return GCMark::testbits(marked, GCMark::WHITEBITS) &&
               !GCMark::testbits(marked, currentWhite);
    }

    void makewhite(GarbageCollector* g, GCObject* o) {
        if (!g || !o) return;

        u8& marked = const_cast<u8&>(o->getMarkedRef());
        u8 currentWhite = g->getCurrentWhiteBits();

        // Clear all color bits and set current white
        marked = (marked & GCMark::MASKMARKS) | currentWhite;
    }

    u8 luaC_white(const GarbageCollector* g) {
        if (!g) return GCMark::WHITE0;
        return g->getCurrentWhiteBits();
    }

    u8 otherwhite(const GarbageCollector* g) {
        if (!g) return GCMark::WHITE1;
        return g->getOtherWhiteBits();
    }

} // namespace GCUtils
} // namespace Lua
