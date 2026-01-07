#include "order_core.h"

bool Order::operator==(const Order &other) const{
    return id == other.id;
}
