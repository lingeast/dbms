/*
 * aggregate.h
 *
 *  Created on: Mar 17, 2014
 *      Author: root
 */

#ifndef AGGREGATE_H_
#define AGGREGATE_H_

template<typename F, typename I>
F mintem(F total, I one)
{
	return  total < one ? total : one * 1.0;
}

template<typename F, typename I>
F maxtem(F total, I one)
{
	return  total > one ? total : one * 1.0;
}

template<typename F, typename I>
F cnttem(F total, I one)
{
	return  total + 1;
}

template<typename F, typename I>
F sumtem(F total, I one)
{
	return  total + one;
}




#endif /* AGGREGATE_H_ */
