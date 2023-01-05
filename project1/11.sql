SELECT trainer.name
FROM caughtpokemon, trainer
WHERE caughtpokemon.owner_id = trainer.id
GROUP BY trainer.name, owner_id, caughtpokemon.pid
HAVING COUNT(*) >= 2
ORDER BY caughtpokemon.owner_id;