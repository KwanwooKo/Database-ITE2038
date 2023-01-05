SELECT DISTINCT trainer.id, trainer.name, caughtpokemon.nickname
FROM trainer, caughtpokemon, (
        SELECT caughtpokemon.owner_id, MAX(caughtpokemon.level)
        FROM caughtpokemon
        GROUP BY owner_id
        ORDER BY owner_id) AS maxcp(owner_id, level)
WHERE trainer.name = ANY(
                SELECT trainer.name
                FROM trainer, caughtpokemon
                WHERE trainer.id = caughtpokemon.owner_id
                GROUP BY trainer.name
                HAVING COUNT(caughtpokemon.owner_id) >= 3)
AND caughtpokemon.owner_id = trainer.id
AND caughtpokemon.level = maxcp.level
AND caughtpokemon.owner_id = maxcp.owner_id
ORDER BY trainer.id, caughtpokemon.nickname;