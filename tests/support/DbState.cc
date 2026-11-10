#include "support/DbState.h"

namespace dbtest {
namespace {
bool g_available = false;
}

bool available() { return g_available; }
void setAvailable(bool value) { g_available = value; }

}  // namespace dbtest
