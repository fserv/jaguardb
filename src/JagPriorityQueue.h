#ifndef _jag_priority_queue_h_
#define _jag_priority_queue_h_

#include "abax.h"
#include "JagHeapSort.h"

// ordered by weight W, K is just any value
template <class K, class W>
class JagPriorityQueue
{
    public:
        JagPriorityQueue( unsigned long capacity=256, JagQueueType type=JAG_MINQUEUE );
        ~JagPriorityQueue();
        void destroy();
        bool push( const K &anyvalue, const W &weight);
        bool peek( K &anyvalue, W &weight );
        bool pop( K &anyvalue, W &weight, bool  doShrink=true );
        bool popAndAddKey( K &popAnyValue, W &popWeight, const K &newAnyValue, const W &newWeight);
        long size() const;
		void clear();
        // void concurrent( bool flag = true );

    protected:
        JagQueueType  _qtype;
        unsigned long _capacity;
        JagHeapData<AbaxPair<W,K> > *_heapData;
        JagHeapSort<AbaxPair<W,K> >  *_heapSort;

        long capacity() const ;
        void resize();
        void shrink();
};

template <class K, class W>
JagPriorityQueue<K,W>::JagPriorityQueue( unsigned long capacity, JagQueueType type )
{
	if ( capacity <1 ) {
		capacity = 256;
	}
	_capacity = capacity;
	_qtype = type;
	_heapData= new JagHeapData<AbaxPair<W,K> > [_capacity];
	_heapSort = new JagHeapSort<AbaxPair<W,K>  > ( ); 

	_heapSort->resetContext();
}

template <class K, class W>
JagPriorityQueue<K,W>::~JagPriorityQueue()
{
	destroy();
}

template <class K, class W>
void JagPriorityQueue<K,W>::destroy()
{
	d("s208834 JagPriorityQueue destroy()\n" );
	if ( _heapData ) {
		delete [] _heapData;
		d("s02036 delete _heapData\n");
		_heapData = NULL;
	}

	if ( _heapSort ) {
		delete _heapSort;
		d("s02037 delete _heapSort\n");
		_heapSort = NULL;
	}
}

template <class K, class W>
bool JagPriorityQueue<K,W>::push( const K &key, const W &weight)
{
 	if ( _heapSort->_heapContext.heap_len >= _capacity-1 ) {
				resize();
	}

	AbaxPair<W,K> pair(weight, key); 
	JagHeapData<AbaxPair<W,K> > keyval; 
	keyval.keyval = pair;

	if ( _qtype == JAG_MINQUEUE ) {
		_heapSort->_heapInsertKeyMin( _heapData, keyval );
	} else {
		_heapSort->_heapInsertKeyMax( _heapData, keyval );
	}
	return true;
}

template <class K, class W>
bool JagPriorityQueue<K,W>::peek( K &key, W &weight )
{
	if ( _heapSort->_heapContext.heap_len < 1 ) return false;
	key = _heapData[0].keyval.value;
	weight = _heapData[0].keyval.key;

	return true;
}

template <class K, class W>
bool JagPriorityQueue<K,W>:: pop( K &key, W &weight, bool doShrink )
{
	if ( _heapSort->_heapContext.heap_len < 1 ) return false;
	weight = _heapData[0].keyval.key;
	key = _heapData[0].keyval.value;

   	_heapData[0] = _heapData[ _heapSort->_heapContext.heap_len -1 ];
   	_heapSort->_heapContext.heap_len --;

	if ( _qtype == JAG_MINQUEUE ) {
   		_heapSort->_heapMinHeapify( _heapData, 0 ) ;
	} else {
   		_heapSort->_heapMaxHeapify( _heapData, 0 ) ;
	}

	
   	if ( doShrink &&  _heapSort->_heapContext.heap_len < _capacity/8 ) {
		shrink();
	}

	return true;
}

template <class K, class W>
bool JagPriorityQueue<K,W>:: popAndAddKey( K &popKey, W &popWeight, const K &newKey, const W &newWeight)
{
	if ( _heapSort->_heapContext.heap_len < 1 ) return false;
	popWeight = _heapData[0].keyval.key;
	popKey = _heapData[0].keyval.value;

   	_heapData[0].keyval.key = newWeight; 
   	_heapData[0].keyval.value = newKey; 

	if ( _qtype == JAG_MINQUEUE ) {
   		_heapSort->_heapMinHeapify( _heapData, 0 ) ;
	} else {
   		_heapSort->_heapMaxHeapify( _heapData, 0 ) ;
	}

	return true;
}

template <class K, class W>
void JagPriorityQueue<K,W>:: clear() 
{
	K k;
	W w;
	while ( pop(k, w, false) );
}

template <class K, class W>
long JagPriorityQueue<K,W>:: size() const 
{
	return _heapSort->_heapContext.heap_len; 
}

template <class K, class W>
long JagPriorityQueue<K,W>:: capacity() const 
{
	return _capacity;
}


template <class K, class W>
void JagPriorityQueue<K,W>:: resize() 
{
	JagHeapData<AbaxPair<W,K> > *newHeapData = new JagHeapData<AbaxPair<W,K> > [2*_capacity];
	long i;
	for ( i = 0; i < _heapSort->_heapContext.heap_len; ++i ) {
		newHeapData[i] = _heapData[i];
	}
	delete [] _heapData;
	_heapData = newHeapData;
	_capacity = 2*_capacity;
}

template <class K, class W>
void JagPriorityQueue<K,W>:: shrink() 
{
	JagHeapData<AbaxPair<W,K> > *newHeapData = new JagHeapData<AbaxPair<W,K> > [_capacity/2];
	long i;
	for ( i = 0; i < _heapSort->_heapContext.heap_len; ++i ) {
		newHeapData[i] = _heapData[i];
	}
	delete [] _heapData;
	_heapData = newHeapData;
	_capacity = _capacity/2;

}

#endif

