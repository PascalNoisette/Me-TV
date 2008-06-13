#include "thread.h"

Thread::Thread(const Glib::ustring& name)
{
	terminated = true;
	thread = NULL;
	this->name = name;
}

void Thread::start()
{
	Glib::Mutex::Lock lock(mutex);
	if (thread != NULL)
	{
		throw Exception("'" + name + "' thread has already been started");
	}
	
	terminated = false;
	thread = Glib::Thread::create(sigc::mem_fun(*this, &Thread::on_run), true);
}
	
void Thread::on_run()
{		
	TRY
	run();
	CATCH
}
	
void Thread::join(gboolean term)
{
	gboolean do_join = false;
	
	{
		Glib::Mutex::Lock lock(mutex);
		if (thread != NULL)
		{
			if (term)
			{
				terminated = true;
			}
			
			do_join = true;
		}		
	}
	
	if (do_join)
	{
		thread->join();
		thread = NULL;
	}
}
	
void Thread::terminate()
{
	Glib::Mutex::Lock lock(mutex);
	terminated = true;
}
	
gboolean Thread::is_terminated()
{
	Glib::Mutex::Lock lock(mutex);
	return terminated;
}
