SELECT trainer.name, COUNT(*)
FROM trainer, pokemon, caughtpokemon
WHERE trainer.id = caughtpokemon.owner_id
AND pokemon.id = caughtpokemon.pid
GROUP BY trainer.name
HAVING COUNT(DISTINCT pokemon.type) = 2;