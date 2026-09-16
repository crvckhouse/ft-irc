NAME = ft-irc
SRC = main.cpp srcs/server.cpp
OBJS = $(SRC:.cpp=.o)
HEADERS = includes/server.hpp
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) *.gch

fclean: clean
	rm -f $(NAME) a.out

re: fclean all

.PHONY: all clean fclean re
