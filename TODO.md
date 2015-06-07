## World

- Add options to toggle lot subdivision and building styles
- Draw a yellowish ground color (or just use that as the background color if we need no fade)

## Building

- Use a shader to do 3-coloring of the building faces
- Separate types of buildings
  - house
  - urban (fills lot, perhaps with small set back on front and back?)
  - sky scraper
  - warehouse
- Optimize mesh building and rendering.
  - Might need a FloorPlan class so we only build the mesh once.
  - If we had separate roof and wall meshes we could scale walls based on height.
- Expand roof outline to overlap house (soffit)
- More roof styles:
  - sawtooth (warehouses)
  - saltbox
  - gabled
  - gambrel
  - shed

## Lot

- If the building doesn't fit try varying the rotation and/or position 
- Scale building to fit free space
- Track/mark portions of lot that face a road
- Orient buildings towards street

## Block

- Use skeleton to find property line then sub-divide lots.
