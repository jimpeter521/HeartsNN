# HeartsNN
**A Deep Learning implementation of the card game Hearts using TensorFlow.**

This project is inspired by [AlphaGo Zero](https://deepmind.com/blog/alphago-zero-learning-scratch/), and is the project I am using to teach myself Deep Learning concepts. I have found that I learn best when I have to solve real problems. Playing around with toy problems or with thorougly explored example problems (such as [MNIST](https://en.wikipedia.org/wiki/MNIST_database)) doesn't inspire me enough. I learn best when I have a challenging, but not too challenging project.

This project is definitely challenging -- perhaps too challenging. It is still an open question whether I will be able to achieve with Hearts anything close to what DeepMind achieved with Go, Chess, and Shogi. The best performance that I have achieved so far is a NN that by itself can beat consistenly beat random play, but even when supplemented with Monte Carlo search it still can't quite beat the algorithm used to generate its training data (Monte Carlo search using many rollouts with random play).

Yet my main goal has been to teach myself Deep Learning, and I feel that this project has been an great choice for achieving that goal. I am also still hopeful that I'll be able to create a NN model capable of bootstrapping to super-human performance.

This is the first public release of the code. As I say in [PR#1](https://github.com/jimlloyd/HeartsNN/pull/1), I have been working on this code for months -- parts of it go back nearly three decades. I've truncated all of the history here and am starting a fresh repository. But even though this is a fresh start and my first public release, there is still quite a bit of tech debt. I'll be cleaning up the code and adding documentation over time.

I'll eventually announce this project more widely and invite people to play with it and contribute to it if so inclined, but if you are reading this, feel free to reach out to me if you want to get involved in any way.

Jim Lloyd  <jim.lloyd@gmail.com>  
San Francisco, CA  
21 January 2018  
