#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H

// Event Types
enum EventType { LOADSTORE, ALU, COMPAREJUMP };

// Event structure
struct Event {
  float     time;     // Simulated time
  EventType type;     // Event type
};

// Enqueue event e
// Return true if sucessful and false otherwise

bool enqueue(const Event* e);

// Dequeue
// Return the next event on the queue, ordered by time
// Lowest value of time is output first
// Return 0 if queue is empty

const Event* dequeue();

#endif
