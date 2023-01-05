SELECT DISTINCT trainer.name
FROM gym, trainer
WHERE trainer.id = gym.leader_id
ORDER BY trainer.name;