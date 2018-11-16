# HeartsNN
**A Deep Learning implementation of the card game Hearts using TensorFlow.**

Note: status updated below.

This project is inspired by [AlphaGo Zero](https://deepmind.com/blog/alphago-zero-learning-scratch/), and is the project I am using to teach myself Deep Learning concepts. I have found that I learn best when I have to solve real problems. Playing around with toy problems or with thorougly explored example problems (such as [MNIST](https://en.wikipedia.org/wiki/MNIST_database)) doesn't inspire me enough. I learn best when I have a challenging, but not too challenging project.

This project is definitely challenging -- perhaps too challenging. It is still an open question whether I will be able to achieve with Hearts anything close to what DeepMind achieved with Go, Chess, and Shogi. The best performance that I have achieved so far is a NN that by itself can beat consistenly beat random play, but even when supplemented with Monte Carlo search it still can't quite beat the algorithm used to generate its training data (Monte Carlo search using many rollouts with random play). (But see the update below!)

Yet my main goal has been to teach myself Deep Learning, and I feel that this project has been an great choice for achieving that goal. I am also still hopeful that I'll be able to create a NN model capable of bootstrapping to super-human performance.

This is the first public release of the code. As I say in [PR#1](https://github.com/jimlloyd/HeartsNN/pull/1), I have been working on this code for months -- parts of it go back nearly three decades. I've truncated all of the history here and am starting a fresh repository. But even though this is a fresh start and my first public release, there is still quite a bit of tech debt. I'll be cleaning up the code and adding documentation over time.

I'll eventually announce this project more widely and invite people to play with it and contribute to it if so inclined, but if you are reading this, feel free to reach out to me if you want to get involved in any way.

Jim Lloyd  <jim.lloyd@gmail.com>  
San Francisco, CA  
21 January 2018  

# Status update 16 November 2018

This project is now on the back burner, but I realize that I should have updated this README months ago, as I had been able to advance quality of play significantly.

I did achieve a level of learning that lead to improved quality of play. A model trained with one batch of training data could play with intuition alone (no Monte Carlo rollouts) at a level of play very close to the prior model+rollouts used to generate the training data. If I then used that new model for another level of reinforcement learning, the resulting model would outperform the model from prior generation. I saw successive improvements for about 4 generations, at which point play quality had very clearly plateaued.

The final model plays well enough to be quite challenging for my level of play. When I play against three HeartsNN opponents in a game to 100 points, I have a difficult time coming out as the winner. 

One cool attribute of the model is that it's play is entirely deterministic. This means that for a given deal, when all four seats are played by the model the outcome is determined in advance by the deal. This provides a reasonably good measure of how "lucky" a given player's hand is. When I play against three HeartsNN opponennts all using the same model, I can replay a game where the model plays all four positions, and see whether the model played the hand differently that I did. The model chooses the same plays as I do more than half of the time. There is evidence that when we differ that I might make the better choice more often than not, so I am still a somewhat better player than the model. But when playing against model playing the other three seats, the luck of the deals becomes significant, and I will only be able to win if I managed to be dealt enough favorable hands.

I'd love to continue working on this project, but my day job and life have intervened, so the project is currently stalled. If anyone sees this and would like to carry it forward, I'd be happy to assist.

 
 
