

an app that allows users to create papercrafts out of 3D models and display / print them out

recently moved to vulkan

# progress
## Stage 1

### opengl

- [x] show window 
- [x] import fbx object 
- [x] show object on screen 
- [x] manipulation of the object 
- [x] highlight edges shown in viewscreen 
- [x] be able to select edges 

### vulkan

- [x] show window 
- [x] import fbx object 
- [x] show object on screen 
- [x] manipulation of the object 
- [x] highlight edges shown in viewscreen 
- [ ] be able to select edges 


## Stage 2

### opengl


- [x] basic fold non cut edges in code
	- starting with the cut edge, look at the faces that are connected to that edge and look at the other edges on that face, for every "fold" make it 180 degrees 
	- in the first test object this should only be 2 steps ( the two other edges in the thingy)
	- track if a fold has been done before so we dont do it twice (maybe just by checking which folds are already 180)

### vulkan

- [ ] basic fold non cut edges in code
	- starting with the cut edge, look at the faces that are connected to that edge and look at the other edges on that face, for every "fold" make it 180 degrees 
	- in the first test object this should only be 2 steps ( the two other edges in the thingy)
	- track if a fold has been done before so we dont do it twice (maybe just by checking which folds are already 180)


## Stage 3
- [ ] look at edge lengths to make tabs / choose which kind of tab when selecting edges
- [ ] use a Heuristic Cut Optimization system to choose which version of unwrapping works best
- [ ] print button

