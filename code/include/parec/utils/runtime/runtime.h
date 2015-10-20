#pragma once

#include <atomic>
#include <cstdlib>
#include <thread>

#include "parec/utils/functional_utils.h"
#include "parec/utils/printer/arrays.h"

#include "parec/utils/runtime/lock.h"

/**
 * This header provides a header-only implementation of a minimal
 * runtime for nested parallel tasks.
 */

namespace parec {
namespace utils {
namespace runtime {

	struct Worker;

	thread_local static Worker* tl_worker = nullptr;

	static void setCurrentWorker(Worker& worker) {
		tl_worker = &worker;
	}

	static Worker& getCurrentWorker();


	// -----------------------------------------------------------------
	//						Future / Promise
	// -----------------------------------------------------------------

	template<typename T>
	class Future;

	template<>
	class Future<void>;

	template<typename T>
	class Promise;

	template<>
	class Promise<void>;

	template<typename T>
	class FPLink;

	template<>
	class FPLink<void>;


	template<typename T>
	class FPLink {

		friend class Future<T>;

		friend class Promise<T>;

		int ref_counter;

		T value;

		bool done;

	private:

		FPLink() : ref_counter(1), done(false) {}

		FPLink(const T& value)
			: ref_counter(1), value(value), done(true) {}

		void incRef() {
			ref_counter++;
		}

		void decRef() {
			ref_counter--;
			if (ref_counter == 0) delete this;
		}

		bool isDone() const { return done; }

		void setValue(const T& value) {
			this->value = value;
			done = true;
		}

		const T& getValue() const {
			return value;
		}

	};

	template<typename T>
	class Future {

		friend class Promise<T>;

		FPLink<T>* link;

		Future(FPLink<T>* link) : link(link) {
			link->incRef();
		}

	public:

		Future() : link(nullptr) {}

		Future(const T& res) : link(new FPLink<T>(res)) {}

		Future(const Future&) = delete;

		Future(Future&& other) : link(other.link) {
			other.link = nullptr;
		}

		~Future() {
			if (link) link->decRef();
		}

		Future& operator=(const Future&) = delete;

		Future& operator=(Future&& other) {
			if (link == other.link) return *this;
			if (link) link->decRef();
			link = other.link;
			other.link = nullptr;
			return *this;
		}

		bool isDone() const {
			return link->isDone();
		}

		const T& get() const;

	};


	template<typename T>
	class Promise {

		FPLink<T>* link;

	public:

		Promise() : link(new FPLink<T>()) {}

		Promise(const Promise& other) : link(other.link) {
			link->incRef();
		}

		Promise(Promise& other) : link(other.link) {
			other.link = nullptr;
		}

		~Promise() {
			if (link) link->decRef();
		}

		Promise* operator()(const Promise& other) = delete;
		Promise* operator()(Promise&& other) = delete;

		Future<T> getFuture() {
			return Future<T>(link);
		}

		void set(const T& res) {
			link->setValue(res);
		}

	};


	// - void specialization -

	template<>
	class FPLink<void> {

		friend class Future<void>;

		friend class Promise<void>;

		int ref_counter;

		bool done;

	private:

		FPLink(bool done = false) : ref_counter(1), done(done) {}

		void incRef() {
			ref_counter++;
		}

		void decRef() {
			ref_counter--;
			if (ref_counter == 0) delete this;
		}

		bool isDone() const { return done; }

	};

	template<>
	class Future<void> {

		template<typename T>
		friend class Promise;

		FPLink<void>* link;

		Future(FPLink<void>* link) : link(link) {
			link->incRef();
		}

	public:

		Future() : link(nullptr) {}

		Future(const Future&) = delete;

		Future(Future&& other) : link(other.link) {
			other.link = nullptr;
		}

		~Future() {
			if (link) link->decRef();
		}

		Future& operator=(const Future&) = delete;

		Future& operator=(Future&& other) {
			if (link == other.link) return *this;
			if (link) link->decRef();
			link = other.link;
			other.link = nullptr;
			return *this;
		}

		bool isDone() const {
			return !link || link->isDone();
		}

		void get() const;

	};


	template<>
	class Promise<void> {

		FPLink<void>* link;

	public:

		Promise() : link(new FPLink<void>()) {}

		Promise(const Promise& other) : link(other.link) {
			link->incRef();
		}

		Promise(Promise& other) : link(other.link) {
			other.link = nullptr;
		}

		~Promise() {
			if (link) link->decRef();
		}

		Promise* operator()(const Promise& other) = delete;
		Promise* operator()(Promise&& other) = delete;

		Future<void> getFuture() {
			return Future<void>(link);
		}

		void set() {
			link->done = true;
		}

	};



	// -----------------------------------------------------------------
	//						    Worker Pool
	// -----------------------------------------------------------------

	using Task = std::function<void()>;


	template<typename T, size_t size>
	class SimpleQueue {

		static const int bsize = size + 1;

		SpinLock lock;

		std::array<T,bsize> data;

		size_t front;
		size_t back;

	public:

		SimpleQueue() : lock(), front(0), back(0) {
			for(auto& cur : data) cur = 0;
		}

		bool empty() const {
			return front == back;
		}
		bool full() const {
			return ((back + 1) % bsize) == front;
		}

		bool push_front(const T& t) {
			lock.lock();
			if (full()) {
				lock.unlock();
				return false;
			}
			front = (front - 1 + bsize) % bsize;
			data[front] = t;
			lock.unlock();
			return true;
		}

		bool push_back(const T& t) {
			lock.lock();
			if (full()) {
				lock.unlock();
				return false;
			}
			data[back] = t;
			back = (back + 1) % bsize;
			lock.unlock();
			return true;
		}

		T pop_front() {
			lock.lock();
			if (empty()) {
				lock.unlock();
				return 0;
			}
			T res = data[front];
			front = (front + 1) % bsize;
			lock.unlock();
			return res;
		}

		T pop_back() {
			lock.lock();
			if (empty()) {
				lock.unlock();
				return 0;
			}
			back = (back - 1 + bsize) % bsize;
			T res = data[back];
			lock.unlock();
			return res;
		}

		friend std::ostream& operator<<(std::ostream& out, const SimpleQueue& queue) {
			return out << "[" << queue.data << "," << queue.front << " - " << queue.back << "]";
		}

	};

	class WorkerPool;

	struct Worker {

		WorkerPool& pool;

		volatile bool alive;

		SimpleQueue<Task*,8> queue;

		std::thread thread;

	public:

		Worker(WorkerPool& pool)
			: pool(pool), alive(true) { }

		Worker(const Worker&) = delete;
		Worker(Worker&&) = delete;

		Worker& operator=(const Worker&) = delete;
		Worker& operator=(Worker&&) = delete;

		void start() {
			thread = std::thread([&](){ run(); });
		}

		void poison() {
			alive = false;
		}

		void join() {
			thread.join();
		}

	private:

		void run() {

			// register worker
			setCurrentWorker(*this);

			// TODO: idle time handling

			// start processing loop
			while(alive) {
				// conduct a schedule step
				schedule_step();
			}

			// done

		}

	public:

		void schedule_step();

	};




	class WorkerPool {

		std::vector<Worker*> workers;


		WorkerPool() {

			int numWorkers = std::thread::hardware_concurrency();

			// parse environment variable
			if (char* val = std::getenv("NUM_WORKERS")) {
				auto userDef = std::atoi(val);
				if (userDef != 0) numWorkers = userDef;
			}

			// must be at least one
			if (numWorkers < 1) numWorkers = 1;

			// create workers
			for(int i=0; i<numWorkers; ++i) {
				workers.push_back(new Worker(*this));
			}

			// start workers
			for(auto& cur : workers) cur->start();
		}

		~WorkerPool() {
			// shutdown threads

			// poison all workers
			for(auto& cur : workers) {
				cur->poison();
			}

			// wait for their death
			for(auto& cur : workers) {
				cur->join();
			}

			// free resources
			for(auto& cur : workers) {
				delete cur;
			}

		}

	public:

		static WorkerPool& getInstance() {
			static WorkerPool pool;
			return pool;
		}

		int getNumWorkers() const {
			return workers.size();
		}

		Worker& getWorker(int i) {
			return *workers[i];
		}

		Worker& getWorker() {
			return getWorker(rand() % workers.size());
		}

	private:

		template<typename Lambda, typename R>
		struct runner {
			Future<R> operator()(Worker& worker, const Lambda& task) {

				// if the queue is full, process task immediately
				if (worker.queue.full()) {
					return task();  // run task and be happy
				}

				// create a schedulable task
				Promise<R> p;
				auto res = p.getFuture();
				Task* t = new Task([=]() mutable {
					p.set(task());
				});

				// schedule task
				bool succ = worker.queue.push_front(t);
				if (!succ) {
					delete t;
					return task();
				}

				// return future
				return res;
			}
		};

		template<typename Lambda>
		struct runner<Lambda,void> {
			Future<void> operator()(Worker& worker, const Lambda& task) {

				// if the queue is full, process task immediately
				if (worker.queue.full()) {
					task();			// run task and be happy
					return Future<void>();
				}

				// create a schedulable task
				Promise<void> p;
				auto res = p.getFuture();
				Task* t = new Task([=]() mutable {
					task();
					p.set();
				});

				// schedule task
				bool succ = worker.queue.push_front(t);
				if (!succ) {
					delete t;
					task();
					return Future<void>();
				}

				// return future
				return res;

			}
		};

	public:

		template<typename Lambda, typename R>
		Future<R> spawn(const Lambda& lambda) {

			// get current worker
			auto& worker = getCurrentWorker();

			// run task
			return runner<Lambda,R>()(worker, lambda);
		}

	};

	template<typename T>
	const T& Future<T>::get() const {
		while (!isDone()) {
			// process another task
			getCurrentWorker().schedule_step();
		}
		return link->getValue();
	}

	void Future<void>::get() const {
		while (!isDone()) {
			// process another task
			getCurrentWorker().schedule_step();
		}
	}

	inline void Worker::schedule_step() {

		// process a task from the local queue
		if (Task* t = queue.pop_back()) {
			t->operator()();
			delete t;
			return;
		}

		// check that there are other workers
		int numWorker = pool.getNumWorkers();
		if (numWorker <= 1) return;

		// otherwise, steal a task from another worker
		Worker& other = pool.getWorker(rand() % numWorker);
		if (this == &other) {
			schedule_step();
			return;
		}

		if (Task* t = other.queue.pop_front()) {
			t->operator()();
			delete t;
			return;
		}

		// no task found => wait a moment
		cpu_relax();
	}


	static Worker& getCurrentWorker() {
		if (tl_worker) return *tl_worker;
		return WorkerPool::getInstance().getWorker();
	}

	template<
		typename Lambda,
		typename R = typename lambda_traits<Lambda>::result_type
	>
	Future<R> spawn(const Lambda& lambda) {
		return WorkerPool::getInstance().spawn<Lambda,R>(lambda);
	}


} // end namespace runtime
} // end namespace utils
} // end namespace parec
