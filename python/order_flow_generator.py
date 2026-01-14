"""
Models a Sequence of OrderEvents

OrderEvent:
- Unique Identifier
- Timestamp
- Side
- Price
- Quantity
- OrderType
- EventType
"""

#
# Defining our OrderEvent data
#

from enum import Enum

class eOrderSide(Enum):
    BID = 0
    ASK = 1

class eOrderType(Enum):
    LIMIT = 0
    MARKET = 1

class eEventType(Enum):
    NEW = 0
    CANCEL = 1
    AMEND = 2

from dataclasses import dataclass

@dataclass
class OrderEvent:
    uid: int
    timestamp: int
    event_type: eEventType
    side: eOrderSide
    price: int
    quantity: int
    order_type: eOrderType

# 
# Generator
#

import numpy as np

rng = np.random.default_rng(42)

# Reference LOB, source: https://github.com/Kautenja/limit-order-book
import limit_order_book.limit_order_book as lob

ref_lob = lob.LimitOrderBook()

class Generator:
    def __init__(self):
        # Params
        self.tick_size = 100

        # UID: Simple Increment
        self.id_gen = 0

        # Timestamp: Exponential Differences b/w Timestamps
        self.timestamp_float = 0.0 
        self.timestamp_lambda = 2.0

        # EventType: Class RNG
        self.event_type_class_range = [0.5, 0.9, 1.0] # New (50%), Cancel (40%), Amend (20%)

        # Side: 50-50 Chance
        self.bid_side_chance = 0.5

        # Price: Normal Distribution around Mid Price
        self.mid_price = 1.0
        self.price_norm_dist_mean = 1.0
        self.price_norm_dist_std = 0.5

        # Quantity: Log-Normal Distribution (Lots of small orders, rarely large orders)
        self.quantity_lognorm_scale = 100

        # OrderType: Class RNG
        self.limit_order_chance = 0.8
        

    def _next_uid(self, next_event_type: eEventType) -> int:
        
        if next_event_type == eEventType.NEW:
            self.id_gen += 1
        else:
            # Cancel/Amend an Older Order
            id = self.id_gen - np.random.randint(5, 10)
            id = 0 if id < 0 else id

            return id

        return self.id_gen

    def _next_timestamp(self) -> int:
        dt = np.random.exponential(1.0 / self.timestamp_lambda)
        self.timestamp_float += dt

        return int(self.timestamp_float)

    def _next_event_type(self) -> eEventType:
        p = np.random.random()
        type = eEventType.NEW
        if p >= self.event_type_class_range[0] and p <= self.event_type_class_range[1]:
            type = eEventType.CANCEL
        elif p >= self.event_type_class_range[1] and p <= self.event_type_class_range[2]:
            type = eEventType.AMEND
        
        return type

    def _next_side(self) -> eOrderSide:
        p = np.random.random()
        side: eOrderSide = eOrderSide.BID if p > 0.5 else eOrderSide.ASK
        
        return side
    
    def _next_price(self) -> int:
        price_float = np.random.Generator.normal(rng, loc=self.price_norm_dist_mean, scale=self.price_norm_dist_std)

        return int(price_float * self.tick_size)
    
    def _next_quantity(self) -> int:
        qty_float = np.random.Generator.lognormal(rng)

        return int(qty_float * self.quantity_lognorm_scale)
    
    def _next_order_type(self) -> eOrderType:
        p = np.random.random()

        if p < self.limit_order_chance:
            return eOrderType.LIMIT
        return eOrderType.MARKET

    def next_event(self) -> OrderEvent:
        event_type = self._next_event_type()
        uid = self._next_uid(event_type)
        timestamp = self._next_timestamp()
        side = self._next_side()
        price = self._next_price()
        quantity = self._next_quantity()
        order_type = self._next_order_type()
        
        order_event = OrderEvent(uid, timestamp, event_type, side, price, quantity, order_type)

        return order_event


# 
# Testing
# 

gen = Generator()
for i in range(10):
    print(gen.next_event())
