#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <list>

class Event
{
private:
	gulong start_time;
	gulong duration;
public:
	Event(gulong start_time, gulong duration) : start_time(start_time), duration(duration) {}
	gulong get_start_time() const { return start_time; }
	gulong get_end_time() const { return start_time + duration; }
	gulong get_duration() const { return duration; }
	gboolean is_in(gulong at) const { return start_time <= at && at < start_time + duration; }
};

typedef std::list<Event> EventList;

class Scheduler
{
private:
	EventList events;
public:
	EventList& get_events() { return events; }
	
	void add(Event& event)
	{
		events.push_back(event);
	}

	EventList get_at(gulong at) const
	{
		EventList result;

		EventList::const_iterator iterator = events.begin();
		while (iterator != events.end())
		{
			Event event = *iterator;
			if (event.is_in(at))
			{
				result.push_back(event);
			}
		}
		
		return result;
	}
};

#endif
