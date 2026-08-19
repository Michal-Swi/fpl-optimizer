# FPL
[Fantasy Premier League](https://fantasy.premierleague.com/en), great game where you have to 
choose a football squad of 15 and optimize for most points scored. Points are given for different things 
a player can do and substructed for different things. Read up on the site if interested. 

# Data
Data is from [vaastav repo](https://github.com/vaastav/Fantasy-Premier-League) 

# Optimizer
The expected goals, assists etc. are already provided inside the repo, so we are only attributing points and 
optimizing with knapsack-like problems. There are a couple of methods for optimizing FPL, one with taking just the 
top 11 players, as the players not playing don't score points aside from wildcard usage, or taking the top 13
which prevents or heghes risks like injury or red cards. Anyway for my FPL I am currently using top 11 but ts
is not backtested at all and I will change the squad when we'll have the new season data, here is the graph:
![A really really nice graph](https://github.com/Michal-Swi/fpl-optimizer/blob/main/data/graphs/11v13.png))

# Players
Here is a nice graph
![A really nice graph, believe me](https://github.com/Michal-Swi/fpl-optimizer/blob/main/data/graphs/player_value_to_price.png)

# VCPOS
The technique used to write this magnificent optimizer is VCPOS - Vibe Coded Piece Of Shit.
Agents crosscheck each other I am a director I make the code work on my machine and sometimes I change their ideas, 
but AgenticAI proboably would've covered that. 
Great time to JUST be starting a CS degree.  

