SELECT trainer.name, SUM(caughtpokemon.level)
FROM gym, trainer, caughtpokemon
WHERE gym.leader_id = trainer.id
AND caughtpokemon.owner_id = trainer.id
GROUP BY trainer.name
ORDER BY SUM(caughtpokemon.level) desc;