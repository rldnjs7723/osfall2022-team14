## Improve the WRR scheduler
### 1. 프로세스마다 다른 weight 초기값을 갖도록 구현
각 프로세스의 예상 수행 시간의 예측값을 미리 받을 수 있거나 이전 기록을 통해 예측할 수 있다면, 예측값을 바탕으로 최적의 time_slice를 계산하여 weight의 값을 다르게 설정할 수 있을 것입니다.
### 2. task 수가 많을 때 이용, weight의 기본 단위를 조밀하게 구현
task의 수가 적고 weight의 기본 단위가 크면, run queue 간의 total weight 수가 많이 차이남에도 불구하고 load balancing이 잘 일어나지 않게 됩니다. 이는 매우 비효율적이므로, load balancing이 잘 일어날 수 있도록 task 수가 많은 작업에 사용하고, weight의 기본 단위를 잘게 쪼개는 것이 좋겠습니다.
