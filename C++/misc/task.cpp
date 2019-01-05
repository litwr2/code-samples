//Emphasis should be placed on developing code that is both highly efficient AND maintainable.

// Your job is to implement a Matching Engine and the supporting classes to implement a simple exchange.
// The matching engine is just for a single symbol, say "IBM", so there is no need to complicate it beyond that.
// The Matching Engine should support Limit and Market orders being submitted, and Limit orders may also be cancelled
// if they aren't already filled.   Market orders should be either immediately filled, cancelled, or partially filled
// and the remainder cancelled.   They should never rest on the book.   Limit orders should fill any qty they can on
// being submitted, and the remainder of the order should rest on the appropriate side of the book until another order
// comes in that can trade with it.   The order book of resting orders should be maintained in price/time priority
// similar to how most US exchanges work.   When a new order comes in that could possibly trade with multiple resting
// orders, the order at the "best" price should be chosen, and if multiple orders are available at the best price, the
// oldest order should be chosen.   If the new order is bigger in quantity than a resting order, it should continue to
// fill additional resting orders until it is completely filled, or there are no additional resting orders on that side.
// If it is not completely filled, the remaining qty should rest on the book at the order limit price.   Example:
//
// submit_new_order ( order id 1, quantity 100, price 50, side sell, type limit) -> results in
//     Order Ack (order id 1, shares remaining 100)
//
// submit_new_order ( order id 2, quantity 100, price 50, side sell, type Market) -> results in
//     Order Reject (order id 2) //reject because there are no sell orders for it to trade with, only buy orders
//
// submit_new_order ( order id 3, quantity 100, price 49, side sell, type limit) -> results in
//     Order Ack (order id 3, shares remaining 100)
//
// submit_new_order ( order id 4, quantity 100, price 50, side sell, type limit) -> results in
//     Order Ack (order id 4, shares remaining 100)
//
// submit_new_order ( order id 5, quantity 100, price 48, side buy, type limit) -> results in
//     Order Ack (order id 5, shares remaining 100)
//
// The order book now looks something like this:
//     Price Level:   Resting Order Ids:   Side:
//         50:                         1, 4                       sell
//         49:                         3                             sell
//         48:                         5                             buy
//
// submit_new_order ( order id 6, quantity 200, price 51, side buy, type limit) -> results in
//     Fill (order id 3, shares 100, price 49, side sell)
//     Fill (order id 6, shares 100, price 49, side buy)
//     Fill (order id 1, shares 100, price 50, side sell)
//     Fill (order id 6, shares 100, price 50, side buy)
//
// The order book now looks something like this:
//     Price Level:   Resting Order Ids:   Side:
//         50:                         4                             sell
//         48:                         5                             buy
//
// submit_new_order ( order id 7, quantity 200, price 51, side buy, type limit) -> results in
//     Fill (order id 4, shares 100, price 50, side sell)
//     Fill (order id 7, shares 100, price 50, side buy)
//     Order Ack (order id 7, shares remaining 100)
//
// The order book now looks something like this:
//     Price Level:   Resting Order Ids:       Side:
//         51:                         7 (100 remaining) buy
//         48:                         5                                 buy
//
// cancel_existing_order ( order id 5 ) -> results in
//     Cancel (order id 5)
//
// The order book now looks something like this:
//     Price Level:   Resting Order Ids:       Side:
//         51:                         7 (100 remaining) buy
//

#include <climits>
#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <utility>

// This code has a few C++11 constructs, feel free to change to C++98 if needed.

// Assume this class is constructed for you, you don't need to construct it
// or manage its lifetime.
struct Order
{
   enum class Side : char
   {
       Buy,
       Sell
   };

   static std::string to_string(Side s) {
     if (s == Side::Buy)
       return "buy";
     else
       return "sell";
   }

   enum class OrderType : char
   {
       Market,                       // Trade at any price available, reject order if it can't trade immediately
       Limit                           // Trade at the specified price or better, rest order on book if it can't trade immediately
   };

   uint64_t   order_id_;   // Some globally unique identifier for this order, where it comes from is not important
   int64_t     price_;         // Some normalized price type, details aren't important
   uint32_t   quantity_;   // Number of shares to buy or sell
   Side           side_;           // Whether order is to buy or sell
   OrderType type_;           // The order type, limit or market

   static uint64_t order_id;

   Order(int64_t price, uint32_t quantity, Side side, OrderType type) :
     order_id_(++order_id), price_(price), quantity_(quantity), side_(side), type_(type)
   {}

   Order(uint64_t id) : order_id_(id) {}
};

using Side = Order::Side;
using OrderType = Order::OrderType;

uint64_t Order::order_id;

struct OrderBookRecord
{
  uint64_t id_;
  uint32_t total_, remaining_;
  Order::Side side_;
  OrderBookRecord(uint64_t id, uint32_t remaining, uint32_t total, Order::Side side) :
    id_(id), remaining_(remaining), total_(total), side_(side) {}
};

using OrderBook = std::multimap<int64_t, OrderBookRecord>;

OrderBook orderBook;

std::ostream& operator<<(std::ostream &so, const OrderBook& ob)
{
  static const unsigned width1 = 5, width2 = 60;
  std::ostringstream os;
  if (ob.empty()) {
    so << "<The order book is empty>\n";
    return so;
  }
  for (auto i = ob.rbegin(); i != ob.rend(); ++i) {
     os.width(width1);
     os << i->first << " | ";
     auto poi = ob.equal_range(i->first);
     auto p = poi.first;
     for (; p != std::prev(poi.second); ++p, ++i) {
       os << p->second.id_;
       if (p->second.total_ != p->second.remaining_)
         os << " (" << p->second.remaining_<< " remaining)";
       os  << ", ";
     }
     os << p->second.id_;
     if (p->second.total_ != p->second.remaining_)
       os << " (" << p->second.remaining_<< " remaining)";
     while (os.str().length() < width2) os << ' ';
     os << " | " << Order::to_string(i->second.side_) << "\n";
     so << os.str();
     os.str("");
  }
  return so;
}

class Fill
{
   uint64_t   order_id_;
   int64_t    price_;
   uint32_t   shares_;
   Order::Side       side_;
public:
   Fill(uint64_t order_id, int64_t price, uint32_t shares, Order::Side side) :
       order_id_(order_id), price_(price), shares_(shares), side_(side)
   {}
   friend std::ostream& operator<<(std::ostream& so, const Fill &fill) {
     so << "Fill (order id " << fill.order_id_ << ", shares " << fill.shares_ << ", price " << fill.price_
       << ", side " << Order::to_string(fill.side_) << ")\n";
     return so;
   }
};

class Reject
{
   uint64_t   order_id_;
public:
   Reject(uint64_t order_id) : order_id_(order_id) {}
   friend std::ostream& operator<<(std::ostream& so, const Reject &reject) {
     so << "Order Reject (order id " << reject.order_id_ << ")\n";
     return so;
   }
};

class Cancel
{
   uint64_t   order_id_;
public:
   Cancel(uint64_t order_id) : order_id_(order_id) {}
   friend std::ostream& operator<<(std::ostream& so, const Cancel &cancel) {
     so << "Cancel (order id " << cancel.order_id_ << ")\n";
     return so;
   }
};

class OrderAck
{
   uint64_t   order_id_;
   uint32_t   shares_;
public:
   OrderAck(uint64_t order_id, uint32_t shares) :
       order_id_(order_id), shares_(shares)
   {}
   friend std::ostream& operator<<(std::ostream& so, const OrderAck &orderAck) {
     so << "Order Ack (order id " << orderAck.order_id_ << ", shares remaining " << orderAck.shares_ << ")\n";
     return so;
   }
};

struct MessageHub
{
   // You need to call these functions to notify the system of
   // fills, rejects, cancels, and order acknowledgements
   virtual void SendFill(Fill&) = 0;         // Call twice per fill, once for each order that participated in the fill (i.e. the buy and sell orders).
   virtual void SendReject(Reject&) = 0; // Call when a 'market' order can't be filled immediately
   virtual void SendCancel(Cancel&) = 0; // Call when an order is successfully cancelled
   virtual void SendOrderAck(OrderAck&) = 0;// Call when a 'limit' order doesn't trade immediately, but is placed into the book
};

struct MessageHubWork: MessageHub
{
   void SendFill(Fill& fill) { std::cout << fill; }
   void SendReject(Reject& reject) { std::cout << reject; }
   void SendCancel(Cancel& cancel) { std::cout << cancel; }
   void SendOrderAck(OrderAck& orderAck) { std::cout << orderAck; }
};

class MatchingEngine
{
   MessageHub* message_hub_;

public:

   MatchingEngine(MessageHub* message_hub)
       : message_hub_(message_hub)
   {}

   // Implement these functions, and any other supporting functions or classes needed.
   // You can assume these functions will be called by external code, and that they will be used
   // properly, i.e the order objects are valid and filled out correctly.
   // You should call the message_hub_ member to notify it of the various events that happen as a result
   // of matching orders and entering orders into the order book.
   void submit_new_order(Order& order);
   void cancel_existing_order(Order& order);

};

void MatchingEngine::submit_new_order(Order& order) {
  if (order.quantity_ == 0) return;
  uint32_t total = order.quantity_;
  if (order.type_ == Order::OrderType::Limit) {
    if (order.side_ == Order::Side::Sell) {
      auto p = orderBook.rbegin();
      for(;;) {
        if (p == orderBook.rend() || p->first < order.price_) {
          OrderAck orderAck(order.order_id_, order.quantity_);
          message_hub_->SendOrderAck(orderAck);
          orderBook.insert(std::make_pair(order.price_, OrderBookRecord(order.order_id_,
            order.quantity_, total, order.side_)));
          break;
        }
        else if (p->second.side_ == Order::Side::Buy) {
          uint32_t size = std::min(order.quantity_, p->second.remaining_);
          Fill fill = Fill(order.order_id_, p->first, size, Order::Side::Buy);
          message_hub_->SendFill(fill);
          order.quantity_ -= size;
          fill = Fill(p->second.id_, p->first, size, Order::Side::Sell);
          message_hub_->SendFill(fill);
          p->second.remaining_ -= size;
        }
        if (p->second.remaining_ == 0) {
          p++;
          orderBook.erase(p.base());
        }
        else
          ++p;
        if (order.quantity_ == 0) break;
      }
    }
    else {
      auto p = orderBook.begin();
      for(;;) {
        if (p == orderBook.end() || p->first > order.price_) {
          OrderAck orderAck(order.order_id_, order.quantity_);
          message_hub_->SendOrderAck(orderAck);
          orderBook.insert(std::make_pair(order.price_, OrderBookRecord(order.order_id_,
            order.quantity_, total, order.side_)));
          break;
        }
        else if (p->second.side_ == Order::Side::Sell) {
          uint32_t size = std::min(order.quantity_, p->second.remaining_);
          Fill fill = Fill(p->second.id_, p->first, size, Order::Side::Sell);
          message_hub_->SendFill(fill);
          fill = Fill(order.order_id_, p->first, size, Order::Side::Buy);
          message_hub_->SendFill(fill);
          order.quantity_ -= size;
          p->second.remaining_ -= size;
        }
        if (p->second.remaining_ == 0) {
          auto p1 = p++;
          orderBook.erase(p1);
        }
        else
          ++p;
        if (order.quantity_ == 0) break;
      }
    }
  }
  else {
    if (order.side_ == Order::Side::Sell) {
      auto p = orderBook.rbegin();
      for(;;) {
        if (p == orderBook.rend() || p->first < order.price_) {
          Reject reject(order.order_id_);
          message_hub_->SendReject(reject);
          break;
        }
        else if (p->second.side_ == Order::Side::Buy) {
          uint32_t size = std::min(order.quantity_, p->second.remaining_);
          Fill fill = Fill(order.order_id_, p->first, size, Order::Side::Sell);
          message_hub_->SendFill(fill);
          order.quantity_ -= size;
          fill = Fill(p->second.id_, p->first, size, Order::Side::Buy);
          message_hub_->SendFill(fill);
          p->second.remaining_ -= size;
        }
        if (p->second.remaining_ == 0) {
          ++p;
          orderBook.erase(p.base());
        }
        else
          ++p;
        if (order.quantity_ == 0) break;
      }
    }
    else {
      auto p = orderBook.begin();
      for(;;) {
        if (p == orderBook.end() || p->first > order.price_) {
          Reject reject(order.order_id_);
          message_hub_->SendReject(reject);
          break;
        }
        else if (p->second.side_ == Order::Side::Sell) {
          uint32_t size = std::min(order.quantity_, p->second.remaining_);
          Fill fill = Fill(order.order_id_, p->first, size, Order::Side::Buy);
          message_hub_->SendFill(fill);
          order.quantity_ -= size;
          fill = Fill(p->second.id_, p->first, size, Order::Side::Sell);
          message_hub_->SendFill(fill);
          p->second.remaining_ -= size;
        }
        if (p->second.remaining_ == 0) {
          auto p1 = p++;
          orderBook.erase(p1);
        }
        else
          ++p;
        if (order.quantity_ == 0) break;
      }
    }
  }
}

void MatchingEngine::cancel_existing_order(Order& order) {
  for (auto p = orderBook.begin(); p != orderBook.end(); ++p)
    if (order.order_id_ == p->second.id_) {
      Cancel cancel(order.order_id_);
      message_hub_->SendCancel(cancel);
      orderBook.erase(p);
      return;
    }
}

int main() {
  MessageHubWork mhw;
  MatchingEngine m(&mhw);

//The following orders are the same as in the example data above - they should produce the same output as the example data
  Order ord = Order(50, 100, Side::Sell, OrderType::Limit);
  m.submit_new_order(ord);
  m.submit_new_order(ord = Order(50, 100, Side::Sell, OrderType::Market));
  m.submit_new_order(ord = Order(49, 100, Side::Sell, OrderType::Limit));
  m.submit_new_order(ord = Order(50, 100, Side::Sell, OrderType::Limit));
  m.submit_new_order(ord = Order(48, 100, Side::Buy, OrderType::Limit));
  std::cout << orderBook;

  ord = Order(51, 200, Side::Buy, OrderType::Limit);
  m.submit_new_order(ord);
  std::cout << orderBook;

  ord = Order(51, 200, Side::Buy, OrderType::Limit);
  m.submit_new_order(ord);
  std::cout << orderBook;

  ord = Order(5);
  m.cancel_existing_order(ord);
  std::cout << orderBook;

//The following orders are for arbitrary tests
  ord = Order(51, 50, Side::Sell, OrderType::Market);
  m.submit_new_order(ord);
  ord = Order(51, 100, Side::Sell, OrderType::Limit);
  m.submit_new_order(ord);
  std::cout << orderBook;
  for (int i = 1; i < 10; i++) {
    ord = Order(51, 7, Side::Buy, OrderType::Market);
    m.submit_new_order(ord);
  }
  ord = Order(51, 7, Side::Sell, OrderType::Limit);
  m.submit_new_order(ord);
  std::cout << orderBook;
}

