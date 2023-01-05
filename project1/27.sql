SELECT DISTINCT trainer.name, COALESCE(levelabovefifty.level, 0) AS sum
FROM (
    SELECT gym.leader_id, SUM(caughtpokemon.level)
    FROM trainer
    JOIN caughtpokemon ON caughtpokemon.owner_id = trainer.id
    JOIN gym ON gym.leader_id = trainer.id
    WHERE caughtpokemon.level >= 50
    GROUP BY gym.leader_id) AS levelabovefifty(id, level)
RIGHT JOIN (
    SELECT gym.leader_id FROM gym) AS gymleader(id) ON levelabovefifty.id = gymleader.id
JOIN trainer ON trainer.id = gymleader.id
ORDER BY trainer.name;