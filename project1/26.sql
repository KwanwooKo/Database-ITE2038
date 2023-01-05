SELECT trainer.name, pokemon.name, COUNT(*)
FROM trainer, caughtpokemon, pokemon
WHERE trainer.id = caughtpokemon.owner_id
AND pokemon.id = caughtpokemon.pid
AND trainer.name IN (
        SELECT trainer.name
        FROM caughtpokemon, trainer, pokemon
        WHERE trainer.id = caughtpokemon.owner_id
        AND pokemon.id = caughtpokemon.pid
        GROUP BY trainer.name
        HAVING COUNT(DISTINCT pokemon.type) = 1)
GROUP BY trainer.name, pokemon.name
ORDER BY trainer.name, pokemon.name;