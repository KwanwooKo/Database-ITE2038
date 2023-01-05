SELECT t.name, COUNT(cp.owner_id)
FROM trainer AS t, caughtpokemon AS cp
WHERE t.id = cp.owner_id
GROUP BY t.name
HAVING COUNT(cp.owner_id) >= 3
ORDER BY COUNT(cp.owner_id);