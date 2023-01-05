SELECT trainer.name
FROM trainer, caughtpokemon, pokemon
WHERE trainer.id = caughtpokemon.owner_id
AND pokemon.type = 'Water'
AND pokemon.id = caughtpokemon.pid
GROUP BY trainer.name
HAVING COUNT(*) >= 2
ORDER BY trainer.name;