SELECT trainer.name, pokemon.type, COUNT(*)
FROM trainer, pokemon, caughtpokemon
WHERE trainer.id = caughtpokemon.owner_id
AND pokemon.id = caughtpokemon.pid
GROUP BY trainer.name, pokemon.type
ORDER BY trainer.name, pokemon.type;