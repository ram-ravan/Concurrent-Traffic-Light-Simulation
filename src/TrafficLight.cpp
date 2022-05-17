#include <iostream>
#include <random>
#include "TrafficLight.h"
#include <chrono>
#include <future>

using namespace std::chrono_literals; //to support 1s, ms etc. as arguments in chrono library methods

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function.
    std::unique_lock<std::mutex> ulock(_mutex);
    _cond.wait(ulock, [this](){
        return !_queue.empty();
    });
    T currentLight = std::move(_queue.back());
    return currentLight;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> lock(_mutex);
    _queue.emplace_back(std::move(msg));
    _cond.notify_one();
}


/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

TrafficLight::TrafficLightPhase TrafficLight::waitForGreen()
{
    std::lock_guard<std::mutex> lck(_mutex);
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true)
    {
        TrafficLightPhase currentLight = mq->receive();
        if (currentLight == green)
        {
            std::cout << "waitForGreen() has " << _currentPhase << " now! let's return it so that the vehicle can proceed" << std::endl;
            return currentLight;
        }
    }   
}

TrafficLight::TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, 
    // use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread

void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.
    // std::lock_guard<std::mutex> lck(_mutex);
    // _currentPhase = green;
    std::chrono::duration<float> timeSinceLastUpdate; // duration of a single simulation cycle in ms
    std::chrono::time_point<std::chrono::system_clock> start, end;

    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distr(4, 6); // define the range
    int randomSeconds = distr(gen);
    
    while (true)
    {
        std::chrono::seconds timeIntervalSec = std::chrono::duration_cast<std::chrono::seconds>(timeSinceLastUpdate);
        start = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(1ms);
        if (timeIntervalSec.count() >= randomSeconds)
        {
            if (_currentPhase == green)
                _currentPhase = red;
            else if (_currentPhase == red)
                _currentPhase = green;
            timeIntervalSec = 0s;
            timeSinceLastUpdate = 0ms;
            mq->send(std::move(_currentPhase));
        }
        end = std::chrono::high_resolution_clock::now();
        timeSinceLastUpdate += (end - start);
    }
}