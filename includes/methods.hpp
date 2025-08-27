#include "Iresp.hpp"

class Get: public Iresp
{
	public:
		Get();
		~Get();
		void	readSocket();
		void	doTheThing();
};

class Post: public Iresp
{
	public:
		Post();
		~Post();
		void	readSocket();
		void	doTheThing();
};

class Delete: public Iresp
{
	public:
		Delete();
		~Delete();
		void	readSocket();
		void	doTheThing();
};
