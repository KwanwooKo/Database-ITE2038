SELECT DISTINCT trainer.name, levelsum.level
FROM (
        SELECT trainer.name, SUM(caughtpokemon.level)
        FROM pokemon, caughtpokemon, trainer
        WHERE trainer.id = caughtpokemon.owner_id
        AND pokemon.id = caughtpokemon.pid
        GROUP BY trainer.name
        ORDER BY SUM(caughtpokemon.level) desc) AS levelsum(name, level), trainer, caughtpokemon
WHERE trainer.name = levelsum.name
AND caughtpokemon.owner_id = trainer.id
AND levelsum.level = (
        SELECT MAX(levelsum.level)
        FROM (
            SELECT trainer.name, SUM(caughtpokemon.level)
            FROM pokemon, caughtpokemon, trainer
            WHERE trainer.id = caughtpokemon.owner_id
            AND pokemon.id = caughtpokemon.pid
            GROUP BY trainer.name) AS levelsum(name, level), trainer, caughtpokemon, pokemon
        WHERE trainer.id = caughtpokemon.owner_id
        AND pokemon.id = caughtpokemon.pid)
ORDER BY trainer.name;